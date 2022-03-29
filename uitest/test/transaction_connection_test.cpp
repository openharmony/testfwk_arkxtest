/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"
#include "common_utilities_hpp.h"
#include "ipc_transactors.h"

using namespace OHOS::uitest;
using namespace std;

static constexpr uint64_t TIME_DIFF_TOLERANCE_MS = 2;

class DummyTransceiver : public MessageTransceiver {
public:
    bool Initialize() override
    {
        return true;
    }

    void DoEmitMessage(const TransactionMessage &message) override
    {
        emittedMessage_ = message;
    }

public:
    void GetLastEmittedMessage(TransactionMessage &out)
    {
        out = emittedMessage_;
    }

private:
    TransactionMessage emittedMessage_;
};

class MessageTransceiverTest : public testing::Test {
public:
protected:
    void TearDown() override
    {
        transceiver_.Finalize();
        if (asyncWork_.valid()) {
            asyncWork_.get();
        }
    }

    future<void> asyncWork_;
    DummyTransceiver transceiver_;
    static constexpr uint64_t pollTimeoutMs_ = 20;
};

TEST_F(MessageTransceiverTest, checkMessageContent)
{
    auto message = TransactionMessage {};
    transceiver_.EmitCall("a", "b", "c");
    transceiver_.GetLastEmittedMessage(message);
    ASSERT_EQ(TransactionType::CALL, message.type_);
    ASSERT_EQ("a", message.apiId_);
    ASSERT_EQ("b", message.callerParcel_);
    ASSERT_EQ("c", message.paramsParcel_);

    message.id_ = 1234;
    transceiver_.EmitReply(message, "d");
    transceiver_.GetLastEmittedMessage(message);
    ASSERT_EQ(TransactionType::REPLY, message.type_);
    ASSERT_EQ(1234, message.id_); // calling message_id should be kept in the reply
    ASSERT_EQ("d", message.resultParcel_);

    transceiver_.EmitHandshake();
    transceiver_.GetLastEmittedMessage(message);
    ASSERT_EQ(TransactionType::HANDSHAKE, message.type_);

    message.id_ = 5678;
    transceiver_.EmitAck(message);
    transceiver_.GetLastEmittedMessage(message);
    ASSERT_EQ(TransactionType::ACK, message.type_);
    ASSERT_EQ(5678, message.id_); // handshake message_id should be kept in the ack
}

TEST_F(MessageTransceiverTest, enqueueDequeueMessage)
{
    auto message = TransactionMessage {};
    // case1: no message in queue, polling timeout, check status and delayed time
    uint64_t startMs = GetCurrentMillisecond();
    auto status = transceiver_.PollCallReply(message, pollTimeoutMs_);
    uint64_t endMs = GetCurrentMillisecond();
    ASSERT_EQ(MessageTransceiver::PollStatus::ABORT_WAIT_TIMEOUT, status);
    ASSERT_NEAR(pollTimeoutMs_, endMs - startMs, TIME_DIFF_TOLERANCE_MS) << "Incorrect polling time";
    // case2: message in queue, should return immediately
    auto tempMessage = TransactionMessage {.id_ = 1234, .type_=CALL};
    transceiver_.OnReceiveMessage(tempMessage);
    startMs = GetCurrentMillisecond();
    status = transceiver_.PollCallReply(message, pollTimeoutMs_);
    endMs = GetCurrentMillisecond();
    ASSERT_EQ(MessageTransceiver::PollStatus::SUCCESS, status);
    ASSERT_NEAR(endMs, startMs, TIME_DIFF_TOLERANCE_MS) << "Should return immediately";
    ASSERT_EQ(1234, message.id_) << "Incorrect message content";
    // case3. message comes at sometime before timeout, should end polling and return it
    constexpr uint64_t delayMs = 10;
    asyncWork_ = async(launch::async, [this, delayMs, tempMessage]() -> void {
        this_thread::sleep_for(chrono::milliseconds(delayMs));
        this->transceiver_.OnReceiveMessage(tempMessage);
    });
    startMs = GetCurrentMillisecond();
    status = transceiver_.PollCallReply(message, pollTimeoutMs_);
    endMs = GetCurrentMillisecond();
    ASSERT_EQ(MessageTransceiver::PollStatus::SUCCESS, status);
    ASSERT_NEAR(endMs - startMs, delayMs, TIME_DIFF_TOLERANCE_MS) << "Should return soon after message enqueue";
}

TEST_F(MessageTransceiverTest, checkMessageFilter)
{
    auto message = TransactionMessage {.type_=CALL};
    // without filter, message should be accepted
    transceiver_.OnReceiveMessage(message);
    auto status = transceiver_.PollCallReply(message, pollTimeoutMs_);
    ASSERT_EQ(MessageTransceiver::PollStatus::SUCCESS, status);
    auto filter = [](TransactionType type) -> bool {
        return type != TransactionType::CALL;
    };
    // message should be filtered out, poll will be timeout
    transceiver_.SetMessageFilter(filter);
    transceiver_.OnReceiveMessage(message);
    status = transceiver_.PollCallReply(message, pollTimeoutMs_);
    ASSERT_EQ(MessageTransceiver::PollStatus::ABORT_WAIT_TIMEOUT, status);
}

TEST_F(MessageTransceiverTest, checkAnwserHandshakeAtomatically)
{
    auto handshake = TransactionMessage {.id_=1234, .type_=HANDSHAKE};
    transceiver_.OnReceiveMessage(handshake);
    auto emitted = TransactionMessage {};
    transceiver_.GetLastEmittedMessage(emitted);
    // should emit ack automatically on receiving handshake
    ASSERT_EQ(TransactionType::ACK, emitted.type_);
    ASSERT_EQ(handshake.id_, emitted.id_);
}

TEST_F(MessageTransceiverTest, immediateExitHandling)
{
    auto message = TransactionMessage {.type_=TransactionType::EXIT};
    // EXIT-message comes at sometime before timeout, should end polling and return it
    constexpr uint64_t delayMs = 10;
    asyncWork_ = async(launch::async, [this, delayMs, message]() {
        this_thread::sleep_for(chrono::milliseconds(delayMs));
        this->transceiver_.OnReceiveMessage(message);
    });
    const uint64_t startMs = GetCurrentMillisecond();
    const auto status = transceiver_.PollCallReply(message, pollTimeoutMs_);
    const uint64_t endMs = GetCurrentMillisecond();
    ASSERT_EQ(MessageTransceiver::PollStatus::ABORT_REQUEST_EXIT, status);
    ASSERT_NEAR(endMs - startMs, delayMs, TIME_DIFF_TOLERANCE_MS) << "Should return soon after exit-request";
}

TEST_F(MessageTransceiverTest, immediateConnectionDiedHandling)
{
    transceiver_.ScheduleCheckConnection(false);
    // connection died before timeout, should end polling and return it
    auto message = TransactionMessage {};
    const uint64_t startMs = GetCurrentMillisecond();
    auto status = transceiver_.PollCallReply(message, WATCH_DOG_TIMEOUT_MS * 2);
    const uint64_t endMs = GetCurrentMillisecond();
    constexpr uint64_t toleranceMs = WATCH_DOG_TIMEOUT_MS / 100;
    ASSERT_EQ(MessageTransceiver::PollStatus::ABORT_CONNECTION_DIED, status);
    ASSERT_NEAR(endMs - startMs, WATCH_DOG_TIMEOUT_MS, toleranceMs) << "Should return soon after connection died";
}

TEST_F(MessageTransceiverTest, checkScheduleHandshake)
{
    transceiver_.ScheduleCheckConnection(false);
    // connection died before timeout, should end polling and return it
    auto message = TransactionMessage {};
    constexpr uint64_t handshakeDelayMs = 1000;
    asyncWork_ = async(launch::async, [this, handshakeDelayMs, message]() {
        this_thread::sleep_for(chrono::milliseconds(handshakeDelayMs));
        auto handshake = TransactionMessage {.type_=TransactionType::HANDSHAKE};
        this->transceiver_.OnReceiveMessage(handshake);
    });
    const uint64_t startMs = GetCurrentMillisecond();
    const auto status = transceiver_.PollCallReply(message, WATCH_DOG_TIMEOUT_MS * 2);
    const uint64_t endMs = GetCurrentMillisecond();
    // since handshake comes at 1000th ms, connection should die at (1000+WATCH_DOG_TIMEOUT_MS)th ms
    constexpr uint64_t expectedConnDieMs = handshakeDelayMs + WATCH_DOG_TIMEOUT_MS;
    constexpr uint64_t toleranceMs = WATCH_DOG_TIMEOUT_MS / 100;
    ASSERT_EQ(MessageTransceiver::PollStatus::ABORT_CONNECTION_DIED, status);
    ASSERT_NEAR(endMs - startMs, expectedConnDieMs, toleranceMs) << "Incorrect time elapse";
}

TEST_F(MessageTransceiverTest, ensureConnected)
{
    constexpr uint64_t timeoutMs = 100;
    // give no incoming message, should be timed-out
    ASSERT_FALSE(transceiver_.EnsureConnectionAlive(timeoutMs));
    // inject incoming message before timeout, should return true immediately
    static constexpr uint64_t incomingDelayMs = 60;
    asyncWork_ = async(launch::async, [this]() {
        this_thread::sleep_for(chrono::milliseconds(incomingDelayMs));
        auto message = TransactionMessage {.type_=TransactionType::ACK};
        this->transceiver_.OnReceiveMessage(message);
    });
    const uint64_t startMs = GetCurrentMillisecond();
    ASSERT_TRUE(transceiver_.EnsureConnectionAlive(timeoutMs));
    const uint64_t endMs = GetCurrentMillisecond();
    ASSERT_NEAR(startMs, endMs, incomingDelayMs + TIME_DIFF_TOLERANCE_MS); // check return immediately
}

class DummyServer : public TransactionServer {
public:
    bool Initialize() override
    {
        auto func = [](string_view api, string_view caller, string_view params) { return string(api) + "_ok"; };
        SetCallFunction(func); // set custom api-call processing method
        return Transactor::Initialize();
    }

    MessageTransceiver *GetTransceiver()
    {
        return transceiver_.get();
    }

protected:
    unique_ptr<MessageTransceiver> CreateTransceiver() override
    {
        return make_unique<DummyTransceiver>();
    }
};

class TransactionServerTest : public testing::Test {
protected:
    void SetUp() override
    {
        server_.Initialize();
    }

    void TearDown() override
    {
        server_.Finalize();
        if (asyncWork_.valid()) {
            asyncWork_.get();
        }
    }

    future<uint32_t> asyncWork_;
    DummyServer server_;
};

TEST_F(TransactionServerTest, checkRunLoop)
{
    asyncWork_ = async(launch::async, [this]() {
        return this->server_.RunLoop();
    });

    static constexpr size_t testSetSize = 3;
    const array<uint32_t, testSetSize> ids = {1, 2, 3};
    const array<string, testSetSize> apis = {"yz", "zl", "lj"};
    const array<string, testSetSize> expectedReply = {"yz_ok", "zl_ok", "lj_ok"};
    // check call-message handling loop
    auto transceiver = reinterpret_cast<DummyTransceiver *>(server_.GetTransceiver());
    for (size_t idx = 0; idx < testSetSize; idx++) {
        // inject call message
        auto call = TransactionMessage {.id_=ids.at(idx), .type_=TransactionType::CALL, .apiId_=apis.at(idx)};
        transceiver->OnReceiveMessage(call);
        // check the emitted reply message corresponding to the inject call, after a short interval
        this_thread::sleep_for(chrono::milliseconds(TIME_DIFF_TOLERANCE_MS));
        auto reply = TransactionMessage {};
        transceiver->GetLastEmittedMessage(reply);
        ASSERT_EQ(TransactionType::REPLY, reply.type_);
        ASSERT_EQ(ids.at(idx), reply.id_);
        ASSERT_EQ(expectedReply.at(idx), reply.resultParcel_); // check the replied result
    }
    // request exit, should end loop immediately with success code
    auto terminate = TransactionMessage {.type_=TransactionType::EXIT};
    const uint64_t startMs = GetCurrentMillisecond();
    transceiver->OnReceiveMessage(terminate);
    auto exitCode = asyncWork_.get();
    const uint64_t endMs = GetCurrentMillisecond();
    ASSERT_EQ(0, exitCode);
    ASSERT_NEAR(startMs, endMs, TIME_DIFF_TOLERANCE_MS); // check exit immediately
}

TEST_F(TransactionServerTest, checkExitLoopWhenConnectionDied)
{
    // enable connection check and enter loop
    server_.GetTransceiver()->ScheduleCheckConnection(false);
    asyncWork_ = async(launch::async, [this]() {
        return this->server_.RunLoop();
    });
    // given no handshake, should end loop immediately with failure code after timeout
    const uint64_t startMs = GetCurrentMillisecond();
    auto exitCode = asyncWork_.get();
    const uint64_t endMs = GetCurrentMillisecond();
    ASSERT_NE(0, exitCode);
    // check exit immediately after timeout
    ASSERT_NEAR(startMs, endMs, WATCH_DOG_TIMEOUT_MS * 1.02f);
}

class DummyClient : public TransactionClient {
public:

    MessageTransceiver *GetTransceiver()
    {
        return transceiver_.get();
    }

protected:
    unique_ptr<MessageTransceiver> CreateTransceiver() override
    {
        return make_unique<DummyTransceiver>();
    }
};

class TransactionClientTest : public testing::Test {
protected:
    void SetUp() override
    {
        client_.Initialize();
    }

    void TearDown() override
    {
        if (asyncWork_.valid()) {
            // do this to ensure asyncWork_ terminates normally
            auto terminate = TransactionMessage {.type_=TransactionType::EXIT};
            client_.GetTransceiver()->OnReceiveMessage(terminate);
            asyncWork_.get();
        }
        client_.Finalize();
    }

    future<string> asyncWork_;
    DummyClient client_;
};

TEST_F(TransactionClientTest, checkInvokeApi)
{
    static constexpr size_t testSetSize = 3;
    const array<string, testSetSize> apis = {"yz", "zl", "lj"};
    const array<string, testSetSize> mockReplies = {"yz_ok", "zl_ok", "lj_ok"};
    auto transceiver = reinterpret_cast<DummyTransceiver *>(client_.GetTransceiver());
    auto message = TransactionMessage();
    for (size_t idx = 0; idx < testSetSize; idx++) {
        asyncWork_ = async(launch::async, [this, &apis, idx]() -> string {
            return this->client_.InvokeApi(apis.at(idx), "", "");
        });
        // wait and check the emitted call-message
        this_thread::sleep_for(chrono::milliseconds(TIME_DIFF_TOLERANCE_MS));
        transceiver->GetLastEmittedMessage(message);
        ASSERT_EQ(TransactionType::CALL, message.type_);
        ASSERT_EQ(apis.at(idx), message.apiId_);
        // inject reply and check invocation result
        auto mockResult = mockReplies.at(idx);
        auto reply = TransactionMessage {.id_=message.id_, .type_=TransactionType::REPLY, .resultParcel_= mockResult};
        transceiver->OnReceiveMessage(reply);
        this_thread::sleep_for(chrono::milliseconds(TIME_DIFF_TOLERANCE_MS));
        const uint64_t startMs = GetCurrentMillisecond();
        auto actualResult = asyncWork_.get();
        const uint64_t endMs = GetCurrentMillisecond();
        ASSERT_EQ(mockResult, actualResult);
        // check return result immediately
        ASSERT_NEAR(startMs, endMs, TIME_DIFF_TOLERANCE_MS);
    }
}

TEST_F(TransactionClientTest, checkResultWhenConnectionDied)
{
    // enable connection check
    client_.GetTransceiver()->ScheduleCheckConnection(false);
    asyncWork_ = async(launch::async, [this]() -> string {
        return this->client_.InvokeApi("yz", "", "");
    });
    // trigger connection timeout by giving no incoming message, should return error result
    uint64_t startMs = GetCurrentMillisecond();
    auto result = asyncWork_.get();
    uint64_t endMs = GetCurrentMillisecond();
    ASSERT_NE(string::npos, result.find("INTERNAL_ERROR"));
    ASSERT_NE(string::npos, result.find("connection with uitest_daemon is dead"));
    // check return immediately after timeout
    ASSERT_NEAR(startMs, endMs, WATCH_DOG_TIMEOUT_MS * 1.02f);
    // connection is already dead, should return immediately in later invocations
    asyncWork_ = async(launch::async, [this]() -> string {
        return this->client_.InvokeApi("yz", "", "");
    });
    startMs = GetCurrentMillisecond();
    result = asyncWork_.get();
    endMs = GetCurrentMillisecond();
    ASSERT_NE(string::npos, result.find("INTERNAL_ERROR"));
    ASSERT_NE(string::npos, result.find("connection with uitest_daemon is dead"));
    // check return immediately due-to dead connection
    ASSERT_NEAR(startMs, endMs, TIME_DIFF_TOLERANCE_MS);
}

TEST_F(TransactionClientTest, checkRejectConcurrentInvoke)
{
    asyncWork_ = async(launch::async, [this]() -> string {
        return this->client_.InvokeApi("yz", "", "");
    });
    // give a short delay to ensure concurrence
    this_thread::sleep_for(chrono::milliseconds(TIME_DIFF_TOLERANCE_MS));
    uint64_t startMs = GetCurrentMillisecond();
    auto reply = client_.InvokeApi("yz", "", "");
    uint64_t endMs = GetCurrentMillisecond();
    // the second call should return immediately and reject the concurrent invoke
    ASSERT_NE(string::npos, reply.find("USAGE_ERROR"));
    ASSERT_NEAR(startMs, endMs, TIME_DIFF_TOLERANCE_MS);
}

TEST_F(TransactionClientTest, checkResultAfterFinalized)
{
    client_.Finalize();
    uint64_t startMs = GetCurrentMillisecond();
    auto reply = client_.InvokeApi("yz", "", "");
    uint64_t endMs = GetCurrentMillisecond();
    // the second call should return immediately and reject the concurrent invoke
    ASSERT_NE(string::npos, reply.find("INTERNAL_ERROR"));
    ASSERT_NE(string::npos, reply.find("connection with uitest_daemon is dead"));
    ASSERT_NEAR(startMs, endMs, TIME_DIFF_TOLERANCE_MS);
}

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

#include <fcntl.h>
#include <sys/mman.h>
#include "gtest/gtest.h"
#include "ipc_transactor.h"

using namespace OHOS::uitest;
using namespace std;

using PipeFds = int32_t[2];

static void PrintEnd()
{
    cout << "...End...";
}

static void RedirectStdoutToPipe(string_view procName, PipeFds fds)
{
    close(fds[0]);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    dup2(fds[1], STDOUT_FILENO);
    cout << "...Start..." << procName;
    (void)atexit(PrintEnd);
}

static string WaitPidAndReadPipe(pid_t pid, PipeFds fds)
{
    string separatorLine = "============================================\n";
    string output = separatorLine;
    close(fds[1]);
    int32_t status = 0;
    waitpid(pid, &status, 0);
    char buf[1024] = {0};
    auto size = read(fds[0], buf, sizeof(buf));
    output.append(string(buf, size)).append("\n").append(separatorLine);
    return output;
}

struct InitConnTestParams {
    // preset conditions
    bool tokenLegal;
    uint32_t clientDelay;
    uint32_t serverDelay;
    // expected stats
    bool expectConnSuccess;
    // case description
    string_view description;
};

// the enviroment of situations to test
static constexpr InitConnTestParams ParamSuite[] = {
    // connection timeout: 5000ms
    {true, 0000, 0000, true, "NormalConnection"},
    {true, 4000, 0000, true, "NormalConnection-ClientDelayInTolerance"},
    {true, 0000, 4000, true, "NormalConnection-ServerDelayInTolerance"},
    {false, 000, 0000, false, "BadConnection-IllegalToken"},
    {true, 6000, 0000, false, "BadConnection-ClientDelayOutofTolerance"},
    {true, 0000, 6000, false, "BadConnection-ServerDelayOutofTolerance"},
};
static constexpr uint32_t suiteSize = 6U;

class ApiTransactorConnectionTest : public ::testing::TestWithParam<uint32_t> {};

TEST_P(ApiTransactorConnectionTest, testInitConnection)
{
    size_t idx = GetParam();
    const auto params = ParamSuite[idx];
    auto token = string("testInitConnection_#") + string(params.description);
    // fork and run server
    auto pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        this_thread::sleep_for(chrono::milliseconds(params.serverDelay));
        ApiTransactor server(true);
        server.InitAndConnectPeer(token, nullptr);
        exit(0); // call exit explicity, to avoid hangs for unkown reason
    }
    // fork and run client
    int32_t fds[2];
    ASSERT_EQ(pipe(fds), 0);
    pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        RedirectStdoutToPipe(token + "-client", fds);
        // client runs in parent process
        this_thread::sleep_for(chrono::milliseconds(params.clientDelay));
        if (!params.tokenLegal) {
            token.append("_bad");
        }
        ApiTransactor client(false);
        auto success = client.InitAndConnectPeer(token, nullptr);
        ASSERT_EQ(success, params.expectConnSuccess);
        exit(0); // call exit explicity, to avoid hangs for unkown reason
    }
    // receive output of client in main process and do assertions
    auto clientOutput = WaitPidAndReadPipe(pid, fds);
    cout << clientOutput << endl;
    ASSERT_TRUE(clientOutput.find(" FAILED ") == string::npos);
}

INSTANTIATE_TEST_SUITE_P(_, ApiTransactorConnectionTest, testing::Range(0U, suiteSize, 1U));

TEST(ApiTransactorTest, testApiTransaction)
{
    constexpr string_view token = "testAapiTransaction";
    constexpr uint32_t serverLifeMs = 100;
    // fork and run server
    auto pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        auto executor = [](const ApiCallInfo &call, ApiReplyInfo &result) {
            string ret = call.apiId_;
            ret.append("/").append(call.callerObjRef_).append("/");
            ret.append(call.paramList_.at(0).get<string>());
            result.resultValue_ = ret;
            result.exception_ = ApiCallErr(ERR_API_USAGE, "MockedError");
        };
        ApiTransactor server(true);
        server.InitAndConnectPeer(token, executor);
        this_thread::sleep_for(chrono::milliseconds(serverLifeMs));
        exit(0);
    }
    // fork and run client
    int32_t fds[2];
    ASSERT_EQ(pipe(fds), 0);
    pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        RedirectStdoutToPipe("testApiTransaction-client", fds);
        ApiTransactor client(false);
        auto connSuccess = client.InitAndConnectPeer(token, nullptr);
        ASSERT_EQ(connSuccess, true);
        auto call = ApiCallInfo {.apiId_ = "testApi", .callerObjRef_ = "testObject"};
        call.paramList_.emplace_back("testParam");
        ApiReplyInfo result;
        client.Transact(call, result);
        // check1. call_data and reply_data are correctly deleviderd
        ASSERT_EQ(result.resultValue_.type(), nlohmann::detail::value_t::string);
        ASSERT_EQ(result.resultValue_.get<string>(), "testApi/testObject/testParam");
        ASSERT_EQ(result.exception_.code_, ERR_API_USAGE);
        ASSERT_EQ(result.exception_.message_, "MockedError");
        // check2. connection state change after server died
        this_thread::sleep_for(chrono::milliseconds(serverLifeMs << 1)); // wait server expired
        ASSERT_EQ(client.GetConnectionStat(), DISCONNECTED);
        // check3. transaction after connection dead gives connection died error
        client.Transact(call, result);
        ASSERT_EQ(result.exception_.code_, ERR_INTERNAL);
        ASSERT_TRUE(result.exception_.message_.find("ipc connection is dead") != string::npos);
        exit(0);
    }
    // receive output of client in main process and do assertions
    auto clientOutput = WaitPidAndReadPipe(pid, fds);
    cout << clientOutput << endl;
    ASSERT_TRUE(clientOutput.find(" FAILED ") == string::npos);
}

#ifdef MFD_ALLOW_SEALING
TEST(ApiTransactorTest, testHandleFdParam)
{
    constexpr string_view token = "testHandleFdParam";
    constexpr uint32_t serverLifeMs = 100;
    // fork and run server
    auto pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        auto executor = [token](const ApiCallInfo &call, ApiReplyInfo &result) {
            // read the file size and return it
            auto fd = call.paramList_.at(0).get<int32_t>();
            if (fcntl(fd, F_GETFD) == -1 && errno == EBADF) {
                result.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Bad fd");
            } else {
                write(fd, token.data(), token.length());
            }
        };
        ApiTransactor server(true);
        server.InitAndConnectPeer(token, executor);
        this_thread::sleep_for(chrono::milliseconds(serverLifeMs));
        exit(0);
    }
    // fork and run client
    int32_t fds[2];
    ASSERT_EQ(pipe(fds), 0);
    pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        RedirectStdoutToPipe("testHandleFdParam-client", fds);
        ApiTransactor client(false);
        auto connSuccess = client.InitAndConnectPeer(token, nullptr);
        ASSERT_EQ(connSuccess, true);
        auto call = ApiCallInfo {.apiId_ = "testApi"};
        auto fd = memfd_create("dummy_file", MFD_ALLOW_SEALING);
        ASSERT_TRUE(fd > 0) << "Failed to create memfd";
        ftruncate(fd, 0);
        call.paramList_.push_back(fd);
        call.fdParamIndex_ = 0;
        ApiReplyInfo result;
        client.Transact(call, result);
        auto fileSize = lseek(fd, 0, SEEK_END);
        // check get correct file size from reply
        ASSERT_EQ(result.exception_.code_, NO_ERROR);
        ASSERT_EQ(fileSize, token.length());
        exit(0);
    }
    // receive output of client in main process and do assertions
    auto clientOutput = WaitPidAndReadPipe(pid, fds);
    cout << clientOutput << endl;
    ASSERT_TRUE(clientOutput.find(" FAILED ") == string::npos);
}
#endif

TEST(ApiTransactorTest, testDetectConcurrentTransaction)
{
    constexpr string_view token = "testDetectConcurrentTransaction";
    static constexpr uint32_t apiCostMs = 100; // mock the timecost of api invocation
    // fork and run server
    auto pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        auto executor = [](const ApiCallInfo &call, ApiReplyInfo &result) {
            this_thread::sleep_for(chrono::milliseconds(apiCostMs));
        };
        ApiTransactor server(true);
        if (server.InitAndConnectPeer(token, executor)) {
            this_thread::sleep_for(chrono::milliseconds(apiCostMs << 1));
        }
        exit(0);
    }
    // fork and run client
    int32_t fds[2];
    ASSERT_EQ(pipe(fds), 0);
    pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        RedirectStdoutToPipe("testDetectConcurrentTransaction-client", fds);
        ApiTransactor client(false);
        auto connSuccess = client.InitAndConnectPeer(token, nullptr);
        ASSERT_EQ(connSuccess, true);
        auto call0 = ApiCallInfo {.apiId_ = "testApi0"};
        ApiReplyInfo result0;
        auto call1 = ApiCallInfo {.apiId_ = "testApi1"};
        ApiReplyInfo result1;
        auto ft = async(launch::async, [&]() { client.Transact(call0, result0); });
        // delay to ensure invocation(call0,reply0) on going
        this_thread::sleep_for(chrono::milliseconds(apiCostMs >> 1));
        client.Transact(call1, result1);
        ft.get();
        ASSERT_EQ(result0.exception_.code_, NO_ERROR);
        ASSERT_EQ(result1.exception_.code_, ERR_API_USAGE);
        exit(0);
    }
    // receive output of client in main process and do assertions
    auto clientOutput = WaitPidAndReadPipe(pid, fds);
    cout << clientOutput << endl;
    ASSERT_TRUE(clientOutput.find(" FAILED ") == string::npos);
}

// clientDecteServerDie is tested in above case.
// in this test, we need to server in parent process and client in child process
TEST(ApiTransactorTest, testServerDetectClientDeath)
{
    constexpr string_view token = "testServerDetectClientDeath";
    // fork and run client
    auto pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        ApiTransactor client(false);
        client.InitAndConnectPeer(token, nullptr);
        client.Finalize();
        exit(0);
    }
    // fork and run server
    int32_t fds[2];
    ASSERT_EQ(pipe(fds), 0);
    pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        RedirectStdoutToPipe("testServerDetectClientDeath-server", fds);
        ApiTransactor server(true);
        auto success = server.InitAndConnectPeer(token, nullptr);
        ASSERT_EQ(true, success);                          // connected on init
        this_thread::sleep_for(chrono::milliseconds(100)); // wait for client expired
        ASSERT_EQ(server.GetConnectionStat(), DISCONNECTED);
        exit(0);
    }
    // receive output of client in main process and do assertions
    auto clientOutput = WaitPidAndReadPipe(pid, fds);
    cout << clientOutput << endl;
    ASSERT_TRUE(clientOutput.find(" FAILED ") == string::npos);
}
/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "createsession_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include "test_server_client.h"
#include "session_token.h"
#include "test_server_error_code.h"

namespace OHOS {
    using namespace testserver;

    bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
    {
        sptr<ITestServerInterface> iTestServerInterface = TestServerClient::GetInstance().LoadTestServer();
        if (iTestServerInterface == nullptr) {
            return false;
        }
        sptr<SessionToken> sessionToken = new (std::nothrow) SessionToken();
        return iTestServerInterface->CreateSession(*sessionToken) == TEST_SERVER_OK;
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}

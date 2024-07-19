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

#include "setpastedata_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include "test_server_service.h"
namespace OHOS {
    bool DoFuzzTestServerSetPasteData(const uint8_t *data, size_t size)
    {
        if ((data == nullptr) || (size == 0)) {
            return false;
        }
        int32_t saId = 5502;
        bool runOnCreate = false;
        int minRange = 0;
        int maxRange = 200;
        auto testServerService = std::make_shared<OHOS::testserver::TestServerService>(saId, runOnCreate);
        FuzzedDataProvider fdp(data, size);
        uint32_t code = MIN_TRANSACTION_ID + 1;
        std::u16string serviceToken = u"OHOS.testserver.ITestServerInterface";
        MessageParcel dataParcel;
        MessageParcel replyParcel;
        MessageOption option;
        std::vector<uint8_t> buffer = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(minRange, maxRange));
        dataParcel.WriteInterfaceToken(serviceToken);
        dataParcel.WriteBuffer(buffer.data(), buffer.size());
        int32_t result = testServerService->OnRemoteRequest(code, dataParcel, replyParcel, option);
        return result == ERR_NONE;
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DoFuzzTestServerSetPasteData(data, size);
    return 0;
}

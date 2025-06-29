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

#ifndef TEST_SERVER_CLIENT_H
#define TEST_SERVER_CLIENT_H

#include "iremote_object.h"
#include "itest_server_interface.h"
#include <common_event_manager.h>
#include <common_event_subscribe_info.h>
#include "memory.h"

namespace OHOS::testserver {

    class TestServerClient {
    public:
        static TestServerClient &GetInstance();
        sptr<ITestServerInterface> LoadTestServer();
        int32_t SetPasteData(std::string text);
        int32_t ChangeWindowMode(int windowId, uint32_t mode);
        int32_t TerminateWindow(int windowId);
        int32_t MinimizeWindow(int windowId);
        bool PublishCommonEvent(const EventFwk::CommonEventData &event);
        void FrequencyLock();
        int32_t SpDaemonProcess(int daemonCommand, std::string extraInfo);
        int32_t CollectProcessMemory(int32_t &pid, ProcessMemoryInfo &processMemoryInfo);
        int32_t CollectProcessCpu(int32_t &pid, bool isNeedUpdate, ProcessCpuInfo &processCpuInfo);
    private:
        TestServerClient();
        ~TestServerClient() = default;
        void InitLoadState();
        bool WaitLoadStateChange(int32_t systemAbilityId);

        sptr<IRemoteObject> remoteObject_ = nullptr;
        sptr<ITestServerInterface> iTestServerInterface_ = nullptr;
    };
} // namespace OHOS::testserver

#ifdef __cplusplus
extern "C" {
#endif
void FrequencyLockPlugin();
#ifdef __cplusplus
}
#endif

#endif // TEST_SERVER_CLIENT_H
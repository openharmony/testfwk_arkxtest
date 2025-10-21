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

#ifndef TEST_SERVER_SERVICE_H
#define TEST_SERVER_SERVICE_H

#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include "test_server_interface_stub.h"
#include "refbase.h"
#include "system_ability.h"
#include "session_token.h"
#include <common_event_manager.h>
#include <common_event_subscribe_info.h>
#include "cpu_collector.h"

namespace OHOS::testserver {
    class TestServerService : public SystemAbility, public TestServerInterfaceStub {
        DECLARE_SYSTEM_ABILITY(TestServerService);

    public:
        TestServerService(int32_t saId, bool runOnCreate);
        ~TestServerService();

        void StartCallerDetectTimer();
        void KillProcess(const std::string& processName);
        bool RetailDemoAppVerify();
        ErrCode CreateSession(const SessionToken &sessionToken) override;

        // Declare the logical interfaces here
        ErrCode SetPasteData(const std::string& text) override;

        ErrCode ChangeWindowMode(int windowId, uint32_t mode) override;

        ErrCode TerminateWindow(int windowId) override;

        ErrCode MinimizeWindow(int windowId) override;

        ErrCode PublishCommonEvent(const EventFwk::CommonEventData &event, bool &re) override;

        ErrCode FrequencyLock() override;

        ErrCode SpDaemonProcess(int daemonCommand, const std::string& extraInfo) override;

        ErrCode CollectProcessMemory(const int32_t pid, ProcessMemoryInfo &processMemoryInfo) override;

        ErrCode CollectProcessCpu(const int32_t pid, bool isNeedUpdate, ProcessCpuInfo &processCpuInfo) override;

        ErrCode GetValueFromDataShare(const std::string &uri, const std::string &key, std::string &value) override;

    protected:
        void OnStart() override;
        void OnStop() override;
        virtual bool IsRootVersion();
        virtual bool IsDeveloperMode();
        void AddCaller();
        void RemoveCaller();
        int GetCallerCount();
        void DestorySession();
        bool RemoveTestServer();

    private:
        std::atomic<int> callerCount_{0};

        class TestServerProxyDeathRecipient : public IRemoteObject::DeathRecipient {
        public:
            explicit TestServerProxyDeathRecipient(const sptr<TestServerService> &testServerService)
                : testServerService_(testServerService){};
            ~TestServerProxyDeathRecipient() = default;
            void OnRemoteDied(const wptr<IRemoteObject> &object) override;

        private:
            sptr<TestServerService> testServerService_;
        };

        class CallerDetectTimer {
        public:
            explicit CallerDetectTimer(const sptr<TestServerService> &testServerService)
                : testServerService_(testServerService), testServerExit_(false){};
            void Start();
            void Cancel();

        private:
            std::thread thread_;
            sptr<TestServerService> testServerService_;
            bool testServerExit_;
        };

        CallerDetectTimer* callerDetectTimer_;
        std::shared_ptr<HiviewDFX::UCollectUtil::CpuCollector> cpuCollector_;
    };
} // namespace OHOS::testserver

#endif // TEST_SERVER_SERVICE_H
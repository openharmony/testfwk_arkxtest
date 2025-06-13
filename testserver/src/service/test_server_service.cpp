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

#include "test_server_service.h"

#include "iremote_object.h"
#include "system_ability_definition.h"
#include "hilog/log.h"
#include "parameters.h"
#include "iservice_registry.h"
#include "test_server_error_code.h"
#include "pasteboard_client.h"
#include "session_manager_lite.h"
#include "wm_common.h"
#include "ws_common.h"
#include "socperf_client.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include "accesstoken_kit.h"
#include "tokenid_kit.h"
#include "memory_collector.h"

namespace OHOS::testserver {
    // TEST_SERVER_SA_ID
    REGISTER_SYSTEM_ABILITY_BY_ID(TestServerService, TEST_SERVER_SA_ID, false); // SA run on demand

    using namespace std;
    using namespace OHOS::HiviewDFX;
    using namespace OHOS::HiviewDFX::UCollectUtil;
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL_SERVICE = {LOG_CORE, 0xD003110, "TestServerService"};
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL_TIMER = {LOG_CORE, 0xD003110, "CallerDetectTimer"};
    static const int CALLER_DETECT_DURING = 10000;
    static const int START_SPDAEMON_PROCESS = 1;
    static const int KILL_SPDAEMON_PROCESS = 2;
    static const int PARA_START_POSITION = 0;
    static const int PARA_END_POSITION = 4;

    TestServerService::TestServerService(int32_t saId, bool runOnCreate) : SystemAbility(saId, runOnCreate)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called. saId=%{public}d, runOnCreate=%{public}d",
            __func__, saId, runOnCreate);
        StartCallerDetectTimer();
    }

    TestServerService::~TestServerService()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (callerDetectTimer_ == nullptr) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. callerDetectTimer_ is nullptr.", __func__);
            return;
        }
        callerDetectTimer_->Cancel();
    }

    void TestServerService::OnStart()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (!IsRootVersion() && !IsDeveloperMode()) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. System mode is unsatisfied.", __func__);
            return;
        }
        bool res = Publish(this);
        if (!res) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. Publish failed", __func__);
        }
    }

    void TestServerService::OnStop()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        IsDeveloperMode();
    }

    bool TestServerService::IsRootVersion()
    {
        bool debugmode = OHOS::system::GetBoolParameter("const.debuggable", false);
        HiLog::Info(LABEL_SERVICE, "%{public}s. debugmode=%{public}d", __func__, debugmode);
        return debugmode;
    }

    bool TestServerService::IsDeveloperMode()
    {
        bool developerMode = OHOS::system::GetBoolParameter("const.security.developermode.state", false);
        HiLog::Info(LABEL_SERVICE, "%{public}s. developerMode=%{public}d", __func__, developerMode);
        return developerMode;
    }

    void TestServerService::StartCallerDetectTimer()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        callerDetectTimer_ = new CallerDetectTimer(this);
        callerDetectTimer_->Start();
    }

    ErrCode TestServerService::CreateSession(const SessionToken &sessionToken)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        bool result = true;
        try {
            result = sessionToken.AddDeathRecipient(
                sptr<TestServerProxyDeathRecipient>(new TestServerProxyDeathRecipient(this)));
        } catch(...) {
            result = false;
        }
        if (!result) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. AddDeathRecipient FAILD.", __func__);
            DestorySession();
            return TEST_SERVER_ADD_DEATH_RECIPIENT_FAILED;
        }
        AddCaller();
        HiLog::Info(LABEL_SERVICE, "%{public}s. Create session SUCCESS. callerCount=%{public}d",
            __func__, GetCallerCount());
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::SetPasteData(const std::string& text)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto pasteBoardMgr = MiscServices::PasteboardClient::GetInstance();
        pasteBoardMgr->Clear();
        auto pasteData = pasteBoardMgr->CreatePlainTextData(text);
        if (pasteData == nullptr) {
            return TEST_SERVER_CREATE_PASTE_DATA_FAILED;
        }
        int32_t ret = pasteBoardMgr->SetPasteData(*pasteData);
        int32_t successErrCode = 27787264;
        return ret == successErrCode ? TEST_SERVER_OK : TEST_SERVER_SET_PASTE_DATA_FAILED;
    }

    ErrCode TestServerService::PublishCommonEvent(const EventFwk::CommonEventData &event, bool &re)
    {
        if (!EventFwk::CommonEventManager::PublishCommonEvent(event)) {
            HiLog::Info(LABEL_SERVICE, "%{public}s Pulbish commonEvent.", __func__);
            re = false;
        }
        re = true;
        int32_t ret = re ? TEST_SERVER_OK : TEST_SERVER_PUBLISH_EVENT_FAILED;
        return ret;
    }

    void TestServerService::AddCaller()
    {
        callerCount_++;
    }

    void TestServerService::RemoveCaller()
    {
        callerCount_--;
    }

    int TestServerService::GetCallerCount()
    {
        return callerCount_.load();
    }

    void TestServerService::DestorySession()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (callerCount_ == 0) {
            HiLog::Info(LABEL_SERVICE, "%{public}s. No proxy exists. Remove the TestServer", __func__);
            RemoveTestServer();
        } else {
            HiLog::Info(LABEL_SERVICE, "%{public}s. Other proxys exist. Can not remove the TestServer", __func__);
        }
    }

    bool TestServerService::RemoveTestServer()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called. ", __func__);
        sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. Get SystemAbility Manager failed!", __func__);
            return false;
        }
        auto res = samgr->UnloadSystemAbility(TEST_SERVER_SA_ID);
        return res == ERR_OK;
    }

    void TestServerService::TestServerProxyDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (object == nullptr) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. IRemoteObject is NULL.", __func__);
            return;
        }
        testServerService_->RemoveCaller();
        testServerService_->DestorySession();
    }

    void TestServerService::CallerDetectTimer::Start()
    {
        HiLog::Info(LABEL_TIMER, "%{public}s called.", __func__);
        thread_ = thread([this] {
            this_thread::sleep_for(chrono::milliseconds(CALLER_DETECT_DURING));
            HiLog::Info(LABEL_TIMER, "%{public}s. Timer is done.", __func__);
            if (!testServerExit_) {
                testServerService_->DestorySession();
            }
        });
        thread_.detach();
    }

    void TestServerService::CallerDetectTimer::Cancel()
    {
        HiLog::Info(LABEL_TIMER, "%{public}s called.", __func__);
        testServerExit_ = true;
    }

    ErrCode TestServerService::FrequencyLock()
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        int performanceModeId = 9100;
        OHOS::SOCPERF::SocPerfClient::GetInstance().PerfRequest(performanceModeId, "");
        return TEST_SERVER_OK;
    }
    
    static std::string ParseDaemonCommand(const std::string& extraInfo)
    {
        try {
            nlohmann::json json = nlohmann::json::parse(extraInfo);
            std::vector<int> paraIndices;
            for (auto it = json.begin(); it != json.end(); ++it) {
                const std::string& key = it.key();
                if (key.compare(PARA_START_POSITION, PARA_END_POSITION, "para") != 0) {
                    continue;
                }
                try {
                    int index = std::stoi(key.substr(PARA_END_POSITION));
                    paraIndices.push_back(index);
                } catch (const std::exception&) {
                    HiLog::Error(LABEL_SERVICE, "Daemon receive an error param: %{public}s", key.c_str());
                }
            }
            std::sort(paraIndices.begin(), paraIndices.end());
            std::ostringstream oss;

            for (int index : paraIndices) {
                std::string paraKey = "para" + std::to_string(index);
                std::string valueKey = "value" + std::to_string(index);
                if (json.contains(paraKey)) {
                    std::string param = json[paraKey].get<std::string>();
                    std::string value = json.contains(valueKey) ? json[valueKey].get<std::string>() : "";
                    oss << " " << param << " " << value;
                }
            }
            return oss.str();
        } catch (const nlohmann::json::exception& e) {
            HiLog::Error(LABEL_SERVICE, "JSON parse error: %{public}s", e.what());
            return "";
        }
    }

    ErrCode TestServerService::SpDaemonProcess(int daemonCommand, const std::string& extraInfo)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (extraInfo == "") {
            HiLog::Error(LABEL_SERVICE, "%{public}s called. but extraInfo is empty", __func__);
            return TEST_SERVER_SPDAEMON_PROCESS_FAILED;
        }
        std::string params = ParseDaemonCommand(extraInfo);

        if (daemonCommand == START_SPDAEMON_PROCESS) {
            std::string command = std::string("./system/bin/SP_daemon " + params + " &");
            std::system(command.c_str());
        } else if (daemonCommand == KILL_SPDAEMON_PROCESS) {
            const std::string spDaemonProcessName = "SP_daemon";
            KillProcess(spDaemonProcessName);
        }
        return TEST_SERVER_OK;
    }

    void TestServerService::KillProcess(const std::string& processName)
    {
        std::string cmd = "ps -ef | grep -v grep | grep " + processName;
        if (cmd.empty()) {
            return;
        }
        FILE *fd = popen(cmd.c_str(), "r");
        if (fd == nullptr) {
            return;
        }
        char buf[4096] = {'\0'};
        while ((fgets(buf, sizeof(buf), fd)) != nullptr) {
            std::string line(buf);
            HiLog::Info(LABEL_SERVICE, "line %s", line.c_str());
            std::istringstream iss(line);
            std::string field;
            std::string pid = "-1";
            int count = 0;
            while (iss >> field) {
                if (count == 1) {
                    pid = field;
                    break;
                }
                count++;
            }
            HiLog::Info(LABEL_SERVICE, "pid %s", pid.c_str());
            cmd = "kill " + pid;
            FILE *fpd = popen(cmd.c_str(), "r");
            if (pclose(fpd) == -1) {
                HiLog::Info(LABEL_SERVICE, "Error: Failed to close file");
                return;
            }
        }
        if (pclose(fd) == -1) {
            HiLog::Info(LABEL_SERVICE, "Error: Failed to close file");
            return;
        }
    }

    ErrCode TestServerService::CollectProcessMemory(const int32_t pid, ProcessMemoryInfo &processMemoryInfo)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
        CollectResult<ProcessMemory> processMemory = collector->CollectProcessMemory(pid);
        if (processMemory.retCode != 0) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. Collect process memory failed.", __func__);
            return TEST_SERVER_COLLECT_PROCESS_INFO_FAILED;
        }
        processMemoryInfo.pid = processMemory.data.pid;
        processMemoryInfo.name = processMemory.data.name;
        processMemoryInfo.rss = processMemory.data.rss;
        processMemoryInfo.pss = processMemory.data.pss;
        processMemoryInfo.swapPss = processMemory.data.swapPss;
        processMemoryInfo.adj = processMemory.data.adj;
        processMemoryInfo.sharedDirty = processMemory.data.sharedDirty;
        processMemoryInfo.privateDirty = processMemory.data.privateDirty;
        processMemoryInfo.sharedClean = processMemory.data.sharedClean;
        processMemoryInfo.privateClean = processMemory.data.privateClean;
        processMemoryInfo.procState = processMemory.data.procState;
        HiLog::Info(LABEL_SERVICE, "%{public}s. Collect process memory success.", __func__);
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::CollectProcessCpu(const int32_t pid, bool isNeedUpdate, ProcessCpuInfo &processCpuInfo)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (cpuCollector_ == nullptr) {
            cpuCollector_ = CpuCollector::Create();
        }
        CollectResult<ProcessCpuStatInfo> processCpuStatInfo =
                                            cpuCollector_->CollectProcessCpuStatInfo(pid, isNeedUpdate);
        if (processCpuStatInfo.retCode != 0) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. Collect process cpu failed.", __func__);
            return TEST_SERVER_COLLECT_PROCESS_INFO_FAILED;
        }
        processCpuInfo.startTime = processCpuStatInfo.data.startTime;
        processCpuInfo.endTime = processCpuStatInfo.data.endTime;
        processCpuInfo.pid = processCpuStatInfo.data.pid;
        processCpuInfo.minFlt = processCpuStatInfo.data.minFlt;
        processCpuInfo.majFlt = processCpuStatInfo.data.majFlt;
        processCpuInfo.cpuLoad = processCpuStatInfo.data.cpuLoad;
        processCpuInfo.uCpuUsage = processCpuStatInfo.data.uCpuUsage;
        processCpuInfo.sCpuUsage = processCpuStatInfo.data.sCpuUsage;
        processCpuInfo.cpuUsage = processCpuStatInfo.data.cpuUsage;
        processCpuInfo.procName = processCpuStatInfo.data.procName;
        processCpuInfo.threadCount = processCpuStatInfo.data.threadCount;
        HiLog::Info(LABEL_SERVICE, "%{public}s. Collect process cpu success.", __func__);
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::ChangeWindowMode(int windowId, uint32_t mode)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto sceneSessionManager = Rosen::SessionManagerLite::GetInstance().GetSceneSessionManagerLiteProxy();
        HiLog::Info(LABEL_SERVICE, "Begin to updateWindowLayoutById %{public}d, mode: %{public}d.", windowId, mode);
        auto ret = sceneSessionManager->UpdateWindowLayoutById(windowId, mode);
        HiLog::Info(LABEL_SERVICE, "UpdateWindowLayoutById over, ret: %{public}d", ret);
        return ret == OHOS::Rosen::WMError::WM_OK ? TEST_SERVER_OK : TEST_SERVER_OPERATE_WINDOW_FAILED;
    }

    ErrCode TestServerService::TerminateWindow(int windowId)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto sceneSessionManager = Rosen::SessionManagerLite::GetInstance().GetSceneSessionManagerLiteProxy();
        HiLog::Info(LABEL_SERVICE, "Begin to terminateWindow %{public}d", windowId);
        auto ret = sceneSessionManager->TerminateSessionByPersistentId(windowId);
        HiLog::Info(LABEL_SERVICE, "TerminateWindow over, ret: %{public}d", ret);
        return ret == OHOS::Rosen::WMError::WM_OK ? TEST_SERVER_OK : TEST_SERVER_OPERATE_WINDOW_FAILED;
    }

    ErrCode TestServerService::MinimizeWindow(int windowId)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto sceneSessionManager = Rosen::SessionManagerLite::GetInstance().GetSceneSessionManagerLiteProxy();
        HiLog::Info(LABEL_SERVICE, "Begin to minimizeWindow %{public}d", windowId);
        auto ret = sceneSessionManager->PendingSessionToBackgroundByPersistenId(windowId);
        HiLog::Info(LABEL_SERVICE, "MinimizeWindow over, ret: %{public}d", ret);
        return ret == OHOS::Rosen::WSError::WM_OK ? TEST_SERVER_OK : TEST_SERVER_OPERATE_WINDOW_FAILED;
    }
} // namespace OHOS::testserver
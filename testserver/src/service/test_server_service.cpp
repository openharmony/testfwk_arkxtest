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
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "test_server_error_code.h"
#ifdef ARKXTEST_PASTEBOARD_ENABLE
#include "pasteboard_client.h"
#include "paste_data.h"
#endif
#ifdef ARKXTEST_KNUCKLE_ACTION_ENABLE
#include "datashare_helper.h"
#endif
#include "session_manager_lite.h"
#include "wm_common.h"
#include "ws_common.h"
#include "socperf_client.h"
#include "time_service_client.h"
#ifdef ARKXTEST_IMF_ENABLE
#include "input_method_controller.h"
#endif
#ifdef ARKXTEST_FONT_ENABLE
#include "font_manager_inner_api.h"
#endif
#ifdef ARKXTEST_VIEW_MODE_ENABLE
#include "ui_appearance.h"
#endif
#include <nlohmann/json.hpp>
#include <sstream>
#include "accesstoken_kit.h"
#include "os_account_manager.h"
#include "tokenid_kit.h"
#include "memory_collector.h"
#include <fstream>
#include <set>
#include <sys/stat.h>
#include "os_account_manager.h"
#ifdef ARKXTEST_LOCATION_ENABLE
#include "locator.h"
#endif

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
        HiLog::Debug(LABEL_SERVICE, "%{public}s called. saId=%{public}d, runOnCreate=%{public}d",
            __func__, saId, runOnCreate);
        StartCallerDetectTimer();
    }

    TestServerService::~TestServerService()
    {
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
        if (callerDetectTimer_ == nullptr) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. callerDetectTimer_ is nullptr.", __func__);
            return;
        }
        callerDetectTimer_->Cancel();
    }

    void TestServerService::OnStart()
    {
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
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
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
        IsDeveloperMode();
    }

    bool TestServerService::IsRootVersion()
    {
        bool debugmode = OHOS::system::GetBoolParameter("const.debuggable", false);
        HiLog::Debug(LABEL_SERVICE, "%{public}s. debugmode=%{public}d", __func__, debugmode);
        return debugmode;
    }

    bool TestServerService::IsDeveloperMode()
    {
        bool developerMode = OHOS::system::GetBoolParameter("const.security.developermode.state", false);
        HiLog::Debug(LABEL_SERVICE, "%{public}s. developerMode=%{public}d", __func__, developerMode);
        return developerMode;
    }

    void TestServerService::StartCallerDetectTimer()
    {
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
        callerDetectTimer_ = new CallerDetectTimer(this);
        callerDetectTimer_->Start();
    }

    ErrCode TestServerService::CreateSession(const SessionToken &sessionToken)
    {
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
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
        HiLog::Debug(LABEL_SERVICE, "%{public}s. Create session SUCCESS. callerCount=%{public}d",
            __func__, GetCallerCount());
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::SetPasteData(const std::string& text, int32_t& setResult)
    {
#ifdef ARKXTEST_PASTEBOARD_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto pasteBoardMgr = MiscServices::PasteboardClient::GetInstance();
        if (!pasteBoardMgr) {
            HiLog::Error(LABEL_SERVICE, "pasteBoardMgr is nullptr");
            setResult = TEST_SERVER_PASTEBOARD_FAILED;
            return TEST_SERVER_OK;
        }
        pasteBoardMgr->Clear();
        auto pasteData = pasteBoardMgr->CreatePlainTextData(text);
        if (pasteData == nullptr) {
            HiLog::Error(LABEL_SERVICE, "CreatePlainTextData failed");
            setResult = TEST_SERVER_CREATE_PASTE_DATA_FAILED;
            return TEST_SERVER_OK;
        }
        int32_t ret = pasteBoardMgr->SetPasteData(*pasteData);
        int32_t successErrCode = 27787264;
        if (ret == successErrCode) {
            setResult = TEST_SERVER_OK;
            return TEST_SERVER_OK;
        } else {
            HiLog::Error(LABEL_SERVICE, "SetPasteData failed with error: %{public}d", ret);
            setResult = TEST_SERVER_SET_PASTE_DATA_FAILED;
            return TEST_SERVER_OK;
        }
#else
        HiLog::Warn(LABEL_SERVICE, "pasteboard is not supported");
        setResult = TEST_SERVER_NOT_SUPPORTED;
        return TEST_SERVER_OK;
#endif
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
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
        if (callerCount_ == 0) {
            HiLog::Debug(LABEL_SERVICE, "%{public}s. No proxy exists. Remove the TestServer", __func__);
            RemoveTestServer();
        } else {
            HiLog::Debug(LABEL_SERVICE, "%{public}s. Other proxys exist. Can not remove the TestServer", __func__);
        }
    }

    bool TestServerService::RemoveTestServer()
    {
        HiLog::Debug(LABEL_SERVICE, "%{public}s called. ", __func__);
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
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
        if (object == nullptr) {
            HiLog::Error(LABEL_SERVICE, "%{public}s. IRemoteObject is NULL.", __func__);
            return;
        }
        testServerService_->RemoveCaller();
        testServerService_->DestorySession();
    }

    void TestServerService::CallerDetectTimer::Start()
    {
        HiLog::Debug(LABEL_TIMER, "%{public}s called.", __func__);
        thread_ = thread([this] {
            this_thread::sleep_for(chrono::milliseconds(CALLER_DETECT_DURING));
            HiLog::Debug(LABEL_TIMER, "%{public}s. Timer is done.", __func__);
            if (!testServerExit_) {
                testServerService_->DestorySession();
            }
        });
        thread_.detach();
    }

    void TestServerService::CallerDetectTimer::Cancel()
    {
        HiLog::Debug(LABEL_TIMER, "%{public}s called.", __func__);
        testServerExit_ = true;
    }

    ErrCode TestServerService::FrequencyLock()
    {
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
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
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
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
            HiLog::Debug(LABEL_SERVICE, "line %s", line.c_str());
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
            HiLog::Debug(LABEL_SERVICE, "pid %s", pid.c_str());
            cmd = "kill " + pid;
            FILE *fpd = popen(cmd.c_str(), "r");
            if (pclose(fpd) == -1) {
                HiLog::Debug(LABEL_SERVICE, "Error: Failed to close file");
                return;
            }
        }
        if (pclose(fd) == -1) {
            HiLog::Debug(LABEL_SERVICE, "Error: Failed to close file");
            return;
        }
    }

    ErrCode TestServerService::CollectProcessMemory(const int32_t pid, ProcessMemoryInfo &processMemoryInfo)
    {
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
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
        HiLog::Debug(LABEL_SERVICE, "%{public}s. Collect process memory success.", __func__);
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::CollectProcessCpu(const int32_t pid, bool isNeedUpdate, ProcessCpuInfo &processCpuInfo)
    {
        HiLog::Debug(LABEL_SERVICE, "%{public}s called.", __func__);
        if (cpuCollector_ == nullptr) {
            cpuCollector_ = CpuCollector::Create();
        }
        CollectResult<ProcessCpuStatInfo> processCpuStatInfo =
                                            cpuCollector_->CollectProcessCpuStatInfo(pid, isNeedUpdate);
        if (processCpuStatInfo.retCode != 0) {
            // Retry to avoid CPU collection failed caused by non-existent process during cpucollactor_ initialization
            processCpuStatInfo = cpuCollector_->CollectProcessCpuStatInfo(pid, isNeedUpdate);
            if (processCpuStatInfo.retCode != 0) {
                HiLog::Error(LABEL_SERVICE, "%{public}s. Collect process cpu failed.", __func__);
                return TEST_SERVER_COLLECT_PROCESS_INFO_FAILED;
            }
            HiLog::Debug(LABEL_SERVICE, "%{public}s. Retry to collect success.", __func__);
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
        HiLog::Debug(LABEL_SERVICE, "%{public}s. Collect process cpu success.", __func__);
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::ChangeWindowMode(int windowId, uint32_t mode)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto sceneSessionManager = Rosen::SessionManagerLite::GetInstance().GetSceneSessionManagerLiteProxy();
        if (sceneSessionManager == nullptr) {
            HiLog::Error(LABEL_SERVICE, "SceneSessionManagerProxy is nullptr");
            return TEST_SERVER_OPERATE_WINDOW_FAILED;
        }
        HiLog::Info(LABEL_SERVICE, "Begin to updateWindowModeById %{public}d, mode: %{public}d.", windowId, mode);
        auto ret = sceneSessionManager->UpdateWindowModeByIdForUITest(windowId, mode);
        HiLog::Info(LABEL_SERVICE, "updateWindowModeById over, ret: %{public}d", ret);
        return ret == OHOS::Rosen::WMError::WM_OK ? TEST_SERVER_OK : TEST_SERVER_OPERATE_WINDOW_FAILED;
    }

    ErrCode TestServerService::TerminateWindow(int windowId)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto sceneSessionManager = Rosen::SessionManagerLite::GetInstance().GetSceneSessionManagerLiteProxy();
        if (sceneSessionManager == nullptr) {
            HiLog::Error(LABEL_SERVICE, "SceneSessionManagerProxy is nullptr");
            return TEST_SERVER_OPERATE_WINDOW_FAILED;
        }
        HiLog::Info(LABEL_SERVICE, "Begin to terminateWindow %{public}d", windowId);
        auto ret = sceneSessionManager->TerminateSessionByPersistentId(windowId);
        HiLog::Info(LABEL_SERVICE, "TerminateWindow over, ret: %{public}d", ret);
        return ret == OHOS::Rosen::WMError::WM_OK ? TEST_SERVER_OK : TEST_SERVER_OPERATE_WINDOW_FAILED;
    }

    ErrCode TestServerService::MinimizeWindow(int windowId)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto sceneSessionManager = Rosen::SessionManagerLite::GetInstance().GetSceneSessionManagerLiteProxy();
        if (sceneSessionManager == nullptr) {
            HiLog::Error(LABEL_SERVICE, "SceneSessionManagerProxy is nullptr");
            return TEST_SERVER_OPERATE_WINDOW_FAILED;
        }
        HiLog::Info(LABEL_SERVICE, "Begin to minimizeWindow %{public}d", windowId);
        auto ret = sceneSessionManager->PendingSessionToBackgroundByPersistentId(windowId);
        HiLog::Info(LABEL_SERVICE, "MinimizeWindow over, ret: %{public}d", ret);
        return ret == OHOS::Rosen::WSError::WS_OK ? TEST_SERVER_OK : TEST_SERVER_OPERATE_WINDOW_FAILED;
    }

    ErrCode TestServerService::GetValueFromDataShare(const std::string &uri, const std::string &key, std::string &value)
    {
#ifdef ARKXTEST_KNUCKLE_ACTION_ENABLE
        auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            HiLog::Error(LABEL_SERVICE, "samgr is nullptr");
            return TEST_SERVER_DATASHARE_FAILED;
        }
        auto datashareSA = samgr->CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
        if (datashareSA == nullptr) {
            HiLog::Error(LABEL_SERVICE, "datashareSA is null");
            return TEST_SERVER_DATASHARE_FAILED;
        }
        sptr<IRemoteObject> remoteObj = samgr->CheckSystemAbility(TEST_SERVER_SA_ID);
        std::pair<int, std::shared_ptr<DataShare::DataShareHelper>> ret0 =
            DataShare::DataShareHelper::Create(remoteObj, uri, "");
        auto helper = ret0.second;
        if (helper == nullptr) {
            HiLog::Error(LABEL_SERVICE, "dataShareHelper is nullptr");
            return TEST_SERVER_DATASHARE_FAILED;
        }
        string columnKeyWordName = "KEYWORD";
        string columnValueName = "VALUE";
        std::vector<std::string> columns = { columnValueName };
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(columnKeyWordName, key);
        Uri queryUri = Uri(uri + "&key=" + key);
        auto resultSet = helper->Query(queryUri, predicates, columns);
        helper->Release();
        if (resultSet == nullptr) {
            HiLog::Error(LABEL_SERVICE, "resultSet is nullptr");
            return TEST_SERVER_DATASHARE_FAILED;
        }
        int32_t count = 0;
        resultSet->GetRowCount(count);
        if (count == 0) {
            HiLog::Warn(LABEL_SERVICE, "resultSet is empty");
            return TEST_SERVER_OK;
        }
        resultSet->GoToRow(0);
        if (resultSet->GetString(0, value) != 0) {
            HiLog::Error(LABEL_SERVICE, "ret is not ok");
            return TEST_SERVER_DATASHARE_FAILED;
        }
        HiLog::Debug(LABEL_SERVICE, "key = %{public}s, value = %{public}s", key.c_str(), value.c_str());
        resultSet->Close();
#else
        value = "0";
#endif
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::SetTime(int64_t timeMs, int32_t& setTimeResult)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto timeService = MiscServices::TimeServiceClient::GetInstance();
        if (timeService == nullptr) {
            HiLog::Error(LABEL_SERVICE, "timeService is nullptr");
            setTimeResult = TEST_SERVER_TIME_SERVICE_FAILED;
            return TEST_SERVER_OK;
        }
        auto ret = timeService->SetTime(timeMs);
        if (!ret) {
            HiLog::Error(LABEL_SERVICE, "SetTime failed");
            setTimeResult = TEST_SERVER_TIME_SERVICE_FAILED;
            return TEST_SERVER_OK;
        }
        HiLog::Debug(LABEL_SERVICE, "SetTime success");
        return TEST_SERVER_OK;
    }

    bool TestServerService::IsValidTimezoneId(const std::string& timezoneId)
    {
        std::set<std::string> availableIds;
        const char* distroPath = "/system/etc/tzdata_distro/timezone_list.cfg";
        const char* defaultPath = "/system/etc/zoneinfo/timezone_list.cfg";
        struct stat s;
        const char* path = (stat(distroPath, &s) == 0) ? distroPath : defaultPath;
        std::ifstream file = std::ifstream(path);
        if (!file) {
            return true;
        }
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }
            line.resize(line.find_last_not_of("\r\n") + 1);
            availableIds.insert(line);
        }
        if (availableIds.find(timezoneId) != availableIds.end()) {
            return true;
        }
        std::ifstream convertFile("/system/etc/zoneinfo/timezone_convert.txt");
        if (!convertFile) {
            return false;
        }
        while (std::getline(convertFile, line)) {
            if (line.empty()) {
                break;
            }
            size_t pos = line.find(':');
            if (pos != std::string::npos && line.substr(0, pos) == timezoneId) {
                std::string converted = line.substr(pos + 1);
                converted.resize(converted.find_last_not_of("\r\n") + 1);
                return availableIds.find(converted) != availableIds.end();
            }
        }
        return false;
    }

    ErrCode TestServerService::SetTimezone(const std::string& timezoneId, int32_t& setTimezoneResult)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called. timezoneId=%{public}s", __func__, timezoneId.c_str());
        if (!IsValidTimezoneId(timezoneId)) {
            HiLog::Error(LABEL_SERVICE, "Invalid timezone ID: %{public}s", timezoneId.c_str());
            setTimezoneResult = TEST_SERVER_INVALID_TIMEZONE_ID;
            return TEST_SERVER_OK;
        }
        auto timeService = MiscServices::TimeServiceClient::GetInstance();
        if (timeService == nullptr) {
            HiLog::Error(LABEL_SERVICE, "timeService is nullptr");
            setTimezoneResult = TEST_SERVER_TIME_SERVICE_FAILED;
            return TEST_SERVER_OK;
        }
        auto ret = timeService->SetTimeZone(timezoneId);
        if (!ret) {
            HiLog::Error(LABEL_SERVICE, "SetTimezone failed");
            setTimezoneResult = TEST_SERVER_TIME_SERVICE_FAILED;
            return TEST_SERVER_OK;
        }
        HiLog::Debug(LABEL_SERVICE, "SetTimezone success");
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::GetPasteData(std::string& pasteText, int32_t& getResult)
    {
#ifdef ARKXTEST_PASTEBOARD_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called", __func__);
        auto pasteboardClient = OHOS::MiscServices::PasteboardClient::GetInstance();
        if (!pasteboardClient) {
            HiLog::Error(LABEL_SERVICE, "pasteboardClient is nullptr");
            getResult = TEST_SERVER_PASTEBOARD_FAILED;
            return TEST_SERVER_OK;
        }
        int32_t successErrCode = 27787264;
        int32_t emptyDataErrCode = 27787283;
        OHOS::MiscServices::PasteData pasteData;
        int32_t result = pasteboardClient->GetPasteData(pasteData);
        if (result == emptyDataErrCode) {
            HiLog::Debug(LABEL_SERVICE, "Pasteboard is empty");
            pasteText = "";
            getResult = TEST_SERVER_OK;
            return TEST_SERVER_OK;
        }
        if (result != successErrCode) {
            HiLog::Error(LABEL_SERVICE, "GetPasteData failed with error: %{public}d", result);
            getResult = TEST_SERVER_PASTEBOARD_FAILED;
            return TEST_SERVER_OK;
        }
        auto text = pasteData.GetPrimaryText();
        if (!text || text->empty()) {
            HiLog::Debug(LABEL_SERVICE, "Pasteboard is empty");
            pasteText = "";
            getResult = TEST_SERVER_OK;
            return TEST_SERVER_OK;
        }
        pasteText = *text;
        getResult = TEST_SERVER_OK;
        return TEST_SERVER_OK;
#else
        HiLog::Error(LABEL_SERVICE, "Pasteboard feature is not enabled");
        getResult = TEST_SERVER_NOT_SUPPORTED;
        return TEST_SERVER_OK;
#endif
    }

    ErrCode TestServerService::ClearPasteData(int32_t& clearResult)
    {
#ifdef ARKXTEST_PASTEBOARD_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto pasteboardClient = MiscServices::PasteboardClient::GetInstance();
        if (!pasteboardClient) {
            HiLog::Error(LABEL_SERVICE, "pasteboardClient is nullptr");
            clearResult = TEST_SERVER_PASTEBOARD_FAILED;
            return TEST_SERVER_OK;
        }
        pasteboardClient->Clear();
        clearResult = TEST_SERVER_OK;
        return TEST_SERVER_OK;
#else
        HiLog::Warn(LABEL_SERVICE, "pasteboard is not supported");
        clearResult = TEST_SERVER_NOT_SUPPORTED;
        return TEST_SERVER_OK;
#endif
    }

    ErrCode TestServerService::HideKeyboard(int32_t& result)
    {
#ifdef ARKXTEST_IMF_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        int32_t imeHideNoActiveClient = 62;
        int32_t imeHideNoActiveInputType = 65;
        auto controller = OHOS::MiscServices::InputMethodController::GetInstance();
        if (!controller) {
            HiLog::Error(LABEL_SERVICE, "InputMethodController is nullptr");
            result = TEST_SERVER_HIDE_KEYBOARD_FAILED;
            return TEST_SERVER_OK;
        }
        auto ret = controller->HideSoftKeyboard();
        if (ret == imeHideNoActiveClient || ret == imeHideNoActiveInputType) {
            HiLog::Warn(LABEL_SERVICE, "No active IME client");
            result = TEST_SERVER_NO_ACTIVE_IME;
            return TEST_SERVER_OK;
        } else if (ret != 0) {
            HiLog::Error(LABEL_SERVICE, "HideSoftKeyboard failed with error: %{public}d", ret);
            result = TEST_SERVER_HIDE_KEYBOARD_FAILED;
            return TEST_SERVER_OK;
        }
        result = TEST_SERVER_OK;
        return TEST_SERVER_OK;
#else
        HiLog::Warn(LABEL_SERVICE, "IMF feature is not supported");
        result = TEST_SERVER_NOT_SUPPORTED;
        return TEST_SERVER_OK;
#endif
    }

    ErrCode TestServerService::GetUserCounts(int32_t &counts)
    {
        vector<int32_t> activatedOsAccountIds;
        auto ret = OHOS::AccountSA::OsAccountManager::QueryActiveOsAccountIds(activatedOsAccountIds);
        HiLog::Info(LABEL_SERVICE, "QueryActiveOsAccountIds, ret: %{public}d", ret);
        if (ret != 0) {
            counts = 1;
            return TEST_SERVER_ACCOUNTOP_FAILED;
        }
        counts = static_cast<int32_t>(activatedOsAccountIds.size());
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::GetUserIdByDisplayId(int32_t displayId, int32_t &userId)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(displayId, userId);
        if (ret != ERR_OK) {
            HiLog::Error(LABEL_SERVICE, "GetUserIdByDisplay in displayId %{public}d failed,"
                "ret: %{public}d", displayId, ret);
            userId = -1;
            return TEST_SERVER_ACCOUNTOP_FAILED;
        }
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::InstallFont(const std::string& fontPath, int32_t& installResult)
    {
#ifdef ARKXTEST_FONT_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        const std::string prefix = "/data/local/tmp/";
        if (fontPath.empty() || fontPath.find(prefix) != 0) {
            HiLog::Error(LABEL_SERVICE, "Invalid font path");
            installResult = TEST_SERVER_INSTALL_FONT_FAILED;
            return TEST_SERVER_OK;
        }
        std::string fileName = fontPath.substr(prefix.length());
        if (fileName.empty() || fileName.find("..") != std::string::npos) {
            HiLog::Error(LABEL_SERVICE, "Invalid font path");
            installResult = TEST_SERVER_INSTALL_FONT_FAILED;
            return TEST_SERVER_OK;
        }
        std::vector<int32_t> ids;
        if (AccountSA::OsAccountManager::QueryActiveOsAccountIds(ids) != 0 || ids.empty()) {
            HiLog::Error(LABEL_SERVICE, "QueryActiveOsAccountIds failed");
            installResult = TEST_SERVER_INSTALL_FONT_FAILED;
            return TEST_SERVER_OK;
        }
        bool hasSuccess = false;
        bool allAlreadyInstalled = true;
        for (const auto& id : ids) {
            auto ret = Global::FontManager::FontManagerInnerApi::InstallFont(fontPath, id);
            if (ret == FONT_ERROR_ALREADY_INSTALLED) {
                continue;
            } else if (ret == 0) {
                hasSuccess = true;
                allAlreadyInstalled = false;
            } else {
                HiLog::Error(LABEL_SERVICE, "InstallFont failed for id=%{public}d: %{public}d", id, ret);
                installResult = TEST_SERVER_INSTALL_FONT_FAILED;
                return TEST_SERVER_OK;
            }
        }
        installResult = allAlreadyInstalled ? TEST_SERVER_FONT_ALREADY_INSTALLED : TEST_SERVER_OK;
        return TEST_SERVER_OK;
#else
        HiLog::Warn(LABEL_SERVICE, "Font management is not supported");
        installResult = TEST_SERVER_NOT_SUPPORTED;
        return TEST_SERVER_OK;
#endif
    }
 	 
    ErrCode TestServerService::UninstallFont(const std::string& fontName, int32_t& uninstallResult)
    {
#ifdef ARKXTEST_FONT_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        std::vector<int32_t> ids;
        auto osAccountRet = AccountSA::OsAccountManager::QueryActiveOsAccountIds(ids);
        HiLog::Info(LABEL_SERVICE, "QueryActiveOsAccountIds ret=%{public}d, count=%{public}zu",
            osAccountRet, ids.size());
        if (osAccountRet != 0 || ids.empty()) {
            HiLog::Error(LABEL_SERVICE, "QueryActiveOsAccountIds failed: %{public}d", osAccountRet);
            uninstallResult = TEST_SERVER_UNINSTALL_FONT_FAILED;
            return TEST_SERVER_OK;
        }
        bool fontFound = false;
        for (const auto& id : ids) {
            auto ret = Global::FontManager::FontManagerInnerApi::UninstallFont(fontName, id);
            HiLog::Info(LABEL_SERVICE, "UninstallFont for id=%{public}d ret=%{public}d", id, ret);
            if (ret == FONT_ERROR_NOT_FOUND) {
                continue;
            } else if (ret != 0) {
                HiLog::Error(LABEL_SERVICE, "UninstallFont failed for id=%{public}d with error: %{public}d", id, ret);
                uninstallResult = TEST_SERVER_UNINSTALL_FONT_FAILED;
                return TEST_SERVER_OK;
            }
            fontFound = true;
        }
        if (!fontFound) {
            HiLog::Error(LABEL_SERVICE, "Font not found: %{public}s", fontName.c_str());
            uninstallResult = TEST_SERVER_FONT_NOT_FOUND;
            return TEST_SERVER_OK;
        }
        uninstallResult = TEST_SERVER_OK;
        return TEST_SERVER_OK;
#else
        HiLog::Warn(LABEL_SERVICE, "Font management is not supported");
        uninstallResult = TEST_SERVER_NOT_SUPPORTED;
        return TEST_SERVER_OK;
#endif
    }

    ErrCode TestServerService::SetViewMode(const std::string& mode, int32_t& setResult)
    {
#ifdef ARKXTEST_VIEW_MODE_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (mode != "dark" && mode != "light") {
            HiLog::Error(LABEL_SERVICE, "Invalid mode: %{public}s", mode.c_str());
            setResult = TEST_SERVER_SET_VIEW_MODE_FAILED;
            return TEST_SERVER_OK;
        }
        
        OHOS::ArkUi::UiAppearance::DarkMode targetMode = (mode == "dark") ?
            OHOS::ArkUi::UiAppearance::DarkMode::ALWAYS_DARK :
            OHOS::ArkUi::UiAppearance::DarkMode::ALWAYS_LIGHT;
            
        auto ret = OHOS::ArkUi::UiAppearance::UIAppearance::SetDarkMode(targetMode);
        if (ret != OHOS::ArkUi::UiAppearance::UiAppearanceAbilityErrCode::SUCCEEDED) {
            HiLog::Error(LABEL_SERVICE, "SetDarkMode failed with error: %{public}d", ret);
            setResult = TEST_SERVER_SET_VIEW_MODE_FAILED;
            return TEST_SERVER_OK;
        }
        HiLog::Debug(LABEL_SERVICE, "SetViewMode success");
        setResult = TEST_SERVER_OK;
        return TEST_SERVER_OK;
#else
        HiLog::Warn(LABEL_SERVICE, "View mode is not supported");
        setResult = TEST_SERVER_NOT_SUPPORTED;
        return TEST_SERVER_OK;
#endif
    }

    ErrCode TestServerService::GetViewMode(std::string& mode, int32_t& getResult)
    {
#ifdef ARKXTEST_VIEW_MODE_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        OHOS::ArkUi::UiAppearance::DarkMode darkMode;
        auto ret = OHOS::ArkUi::UiAppearance::UIAppearance::GetDarkMode(darkMode);
        if (ret != OHOS::ArkUi::UiAppearance::UiAppearanceAbilityErrCode::SUCCEEDED) {
            HiLog::Error(LABEL_SERVICE, "GetDarkMode failed with error: %{public}d", ret);
            getResult = TEST_SERVER_SET_VIEW_MODE_FAILED;
            return TEST_SERVER_OK;
        }
        switch (darkMode) {
            case OHOS::ArkUi::UiAppearance::DarkMode::ALWAYS_DARK:
                mode = "dark";
                break;
            case OHOS::ArkUi::UiAppearance::DarkMode::ALWAYS_LIGHT:
                mode = "light";
                break;
            case OHOS::ArkUi::UiAppearance::DarkMode::UNKNOWN:
            default:
                mode = "unknown";
                HiLog::Warn(LABEL_SERVICE, "Unknown dark mode value: %{public}d", darkMode);
                break;
        }
        getResult = TEST_SERVER_OK;
        return TEST_SERVER_OK;
#else
        HiLog::Warn(LABEL_SERVICE, "View mode is not supported");
        getResult = TEST_SERVER_NOT_SUPPORTED;
        return TEST_SERVER_OK;
#endif
    }

    ErrCode TestServerService::EnableLocationMock(int32_t& enableResult)
    {
        enableResult = TEST_SERVER_OK;
#ifdef ARKXTEST_LOCATION_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto locatorClient = Location::Locator::GetInstance();
        if (!locatorClient) {
            HiLog::Error(LABEL_SERVICE, "locatorClient is nullptr");
            enableResult = TEST_SERVER_LOCATION_MOCK_FAILED;
            return TEST_SERVER_OK;
        }
        auto ret = locatorClient->EnableLocationMock();
        if (ret) {
            HiLog::Info(LABEL_SERVICE, "EnableLocationMock completed successfully");
        } else {
            HiLog::Error(LABEL_SERVICE, "EnableLocationMock failed with error: %{public}d", ret);
            enableResult = TEST_SERVER_LOCATION_MOCK_FAILED;
        }
#else
        HiLog::Warn(LABEL_SERVICE, "Location mock is not supported");
        enableResult = TEST_SERVER_NOT_SUPPORTED;
#endif
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::DisableLocationMock(int32_t& disableResult)
    {
        disableResult = TEST_SERVER_OK;
#ifdef ARKXTEST_LOCATION_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        auto locatorClient = Location::Locator::GetInstance();
        if (!locatorClient) {
            HiLog::Error(LABEL_SERVICE, "locatorClient is nullptr");
            disableResult = TEST_SERVER_LOCATION_MOCK_FAILED;
            return TEST_SERVER_OK;
        }
        auto ret = locatorClient->DisableLocationMock();
        if (ret) {
            HiLog::Info(LABEL_SERVICE, "DisableLocationMock completed successfully");
        } else {
            HiLog::Error(LABEL_SERVICE, "DisableLocationMock failed with error: %{public}d", ret);
            disableResult = TEST_SERVER_LOCATION_MOCK_FAILED;
        }
#else
        HiLog::Warn(LABEL_SERVICE, "Location mock is not supported");
        disableResult = TEST_SERVER_NOT_SUPPORTED;
#endif
        return TEST_SERVER_OK;
    }

    ErrCode TestServerService::SetMockedLocations(const std::vector<TestServerLocation>& locations,
                                                  int32_t timeInterval, int32_t& setResult)
    {
        setResult = TEST_SERVER_OK;
#ifdef ARKXTEST_LOCATION_ENABLE
        HiLog::Info(LABEL_SERVICE, "%{public}s called with %{public}zu locations, interval: %{public}d", __func__,
            locations.size(), timeInterval);
        if (timeInterval < 1) {
            HiLog::Error(LABEL_SERVICE, "Invalid time interval: %{public}d", timeInterval);
            setResult = TEST_SERVER_SET_LOCATIONS_FAILED;
            return TEST_SERVER_OK;
        }
        auto locatorClient = Location::Locator::GetInstance();
        if (!locatorClient) {
            HiLog::Error(LABEL_SERVICE, "locatorClient is nullptr");
            setResult = TEST_SERVER_SET_LOCATIONS_FAILED;
            return TEST_SERVER_OK;
        }
        std::vector<std::shared_ptr<OHOS::Location::Location>> nativeLocations;
        for (const auto& testLoc : locations) {
            std::shared_ptr<OHOS::Location::Location> nativeLoc = std::make_shared<OHOS::Location::Location>();
            nativeLoc->SetLatitude(testLoc.latitude);
            nativeLoc->SetLongitude(testLoc.longitude);
            nativeLoc->SetAltitude(testLoc.altitude);
            nativeLoc->SetAccuracy(testLoc.accuracy);
            nativeLoc->SetSpeed(testLoc.speed);
            nativeLoc->SetDirection(testLoc.direction);
            nativeLoc->SetTimeStamp(testLoc.timeStamp);
            nativeLoc->SetTimeSinceBoot(testLoc.timeSinceBoot);
            nativeLoc->SetIsFromMock(testLoc.isFromMock);
            nativeLocations.push_back(nativeLoc);
        }
        auto ret = locatorClient->SetMockedLocations(timeInterval, nativeLocations);
        if (ret) {
            HiLog::Info(LABEL_SERVICE, "SetMockedLocations completed successfully");
        } else {
            HiLog::Error(LABEL_SERVICE, "SetMockedLocations failed");
            setResult = TEST_SERVER_SET_LOCATIONS_FAILED;
        }
#else
        HiLog::Warn(LABEL_SERVICE, "Location mock is not supported");
        setResult = TEST_SERVER_NOT_SUPPORTED;
#endif
        return TEST_SERVER_OK;
    }
} // namespace OHOS::testserver
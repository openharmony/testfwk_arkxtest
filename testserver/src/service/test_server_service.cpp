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
#include "socperf_client.h"
#include <sstream>
namespace OHOS::testserver {
    // TEST_SERVER_SA_ID
    REGISTER_SYSTEM_ABILITY_BY_ID(TestServerService, TEST_SERVER_SA_ID, false); // SA run on demand

    using namespace std;
    using namespace OHOS::HiviewDFX;
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL_SERVICE = {LOG_CORE, 0xD003110, "TestServerService"};
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL_TIMER = {LOG_CORE, 0xD003110, "CallerDetectTimer"};
    static const int CALLER_DETECT_DURING = 10000;
    static const int START_SPDAEMON_PROCESS = 1;
    static const int KILL_SPDAEMON_PROCESS = 2;

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

    static std::string Trim(const std::string& str) 
    {
        // Trim spaces first
        size_t first = str.find_first_not_of(" \t");
        if (std::string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t");
        std::string trimmed = str.substr(first, (last - first + 1));
        
        // Then trim quotes if present
        if (trimmed.length() >= 2 && trimmed.front() == '"' && trimmed.back() == '"')
            return trimmed.substr(1, trimmed.length() - 2);
        
        return trimmed;
    }

    static std::map<std::string, std::string> GetJsonKVMap(std::string content)
    {
        std::vector<std::string> keyValues;
        std::stringstream ss(content);
        std::string kvPair;

        while (std::getline(ss, kvPair, ',')) {
            keyValues.push_back(kvPair);
        }

        std::map<std::string, std::string> kvMap;
        for (const auto& kv : keyValues) {
            size_t colonPos = kv.find(':');
            if (colonPos != std::string::npos) {
                // 去除引号与空格
                std::string key = Trim(kv.substr(0, colonPos));
                std::string value = Trim(kv.substr(colonPos + 1));
                kvMap[key] = value;
            }
        }

        return kvMap;
    }

    static std::vector<int> GetDaemonParaParaIndices(std::map<std::string, std::string> kvMap)
    {
        std::vector<int> paraIndices;
        for (const auto& [key, _] : kvMap) {
            if (key.compare(0, 4, "para") == 0) {
                try {
                    int index = std::stoi(key.substr(4));
                    paraIndices.push_back(index);
                } catch (const std::exception&) {
                    HiLog::Error(LABEL_SERVICE, "Daemon receive a error param: %{public}s", key.c_str());
                }
            }
        }
        std::sort(paraIndices.begin(), paraIndices.end());

        return paraIndices;
    }

    static std::string ParseDaemonCommand(const std::string& extraInfo) 
    {
        std::string processed;
        for (char c : extraInfo) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                processed += c;
            }
        }

        if (processed.empty() || processed.front() != '{' || processed.back() != '}') {
            return "";
        }

        std::string content = processed.substr(1, processed.length() - 2);
        std::map<std::string, std::string> kvMap = GetJsonKVMap(content);
        std::ostringstream oss;
        bool isFirst = true;
        for (int index : GetDaemonParaParaIndices(kvMap)) {
            std::string paraKey = "para" + std::to_string(index);
            std::string valueKey = "value" + std::to_string(index);

            if (kvMap.find(paraKey) != kvMap.end()) {
                std::string param = kvMap[paraKey];
                std::string value = "";

                if (kvMap.find(valueKey) != kvMap.end()) {
                    value = kvMap[valueKey];
                }

                if (!isFirst) {
                    oss << " ";
                }
                oss << param;

                if (!value.empty()) {
                    oss << " " << value;
                }

                isFirst = false;
            }
        }

        return oss.str();
    }

    ErrCode TestServerService::SpDaemonProcess(int daemonCommand, const std::string& extraInfo)
    {
        HiLog::Info(LABEL_SERVICE, "%{public}s called.", __func__);
        if (extraInfo == "") {
            HiLog::Error(LABEL_SERVICE, "%{public}s called. but extraInfo is empty", __func__);
        }
        std::string params = ParseDaemonCommand(extraInfo);

        if (daemonCommand == START_SPDAEMON_PROCESS) {
            std::string command = std::string("./system/bin/SP_daemon " + token + " &");
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

} // namespace OHOS::testserver
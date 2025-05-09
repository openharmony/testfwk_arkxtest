/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <fstream>
#include <sstream>
#include <csignal>
#include "data_collection.h"

namespace OHOS::perftest {
    using namespace std;
    const string BUNDLE_NAME_PARAM = "BUNDLE_NAME";

    void DataCollection::SetPid(int32_t pid)
    {
        pid_ = pid;
    }

    void DataCollection::SetBundleName(string bundleName)
    {
        bundleName_ = bundleName;
    }

    int32_t DataCollection::GetPidByBundleName(string bundleName)
    {
        string getPidCmd = "pidof " + bundleName_;
        string executeRes;
        int32_t pid = -1;
        if (!ExecuteCommand(getPidCmd, executeRes)) {
            return pid;
        }
        LOG_I("Pid of process %{public}s is %{public}s", bundleName_.c_str(), executeRes.c_str());
        stringstream pidStream(executeRes);
        pidStream >> pid;
        return pid;
    }

    bool DataCollection::IsProcessExist(int32_t pid)
    {
        std::string cmd = "ps -ef | grep -v grep | grep " + to_string(pid);
        FILE *fd = popen(cmd.c_str(), "r");
        if (fd == nullptr) {
            LOG_E("Execute popen to get process failed");
            return false;
        }
        char buf[4096] = {'\0'};
        while ((fgets(buf, sizeof(buf), fd)) != nullptr) {
            std::string line(buf);
            std::istringstream iss(line);
            std::string field;
            int count = 0;
            while (iss >> field) {
                if (count == 1 && field == to_string(pid)) {
                    pclose(fd);
                    return true;
                }
                count++;
            }
        }
        pclose(fd);
        return false;
    }

    bool DataCollection::ExecuteCommand(const string command, string& result)
    {
        LOG_I("Execute command: %{public}s", command.c_str());
        result = "";
        FILE *fp = popen(command.c_str(), "r");
        if (fp == nullptr) {
            LOG_E("Execute popen to execute command: %{public}s failed", command.c_str());
            return false;
        }
        char bufferInfo[1024] = { '\0' };
        while (fgets(bufferInfo, sizeof(bufferInfo), fp) != nullptr) {
            result += bufferInfo;
        }
        pclose(fp);
        LOG_I("Execute command res: %{public}s", result.c_str());
        return true;
    }

    void TimeListener::OnEvent(shared_ptr<HiSysEventRecord> record)
    {
        LOG_I("TimeListener::OnEvent called");
        if (record == nullptr) {
            return;
        }
        string bundleName = "";
        if (record->GetParamValue(BUNDLE_NAME_PARAM, bundleName) != 0) {
            LOG_E("Get the bundlename of HiSysEventRecord failed");
            return;
        }
        if (bundleName != bundleName_) {
            LOG_I("Not the specific bundlename");
            return;
        }
        lastEvent_ = record;
    }

    void TimeListener::OnServiceDied()
    {
        LOG_E("Service disconnect");
    }
    
    shared_ptr<HiviewDFX::HiSysEventRecord> TimeListener::GetEvent()
    {
        return lastEvent_;
    }
} // namespace OHOS::perftest

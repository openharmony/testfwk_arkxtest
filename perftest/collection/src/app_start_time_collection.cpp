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

#include <thread>
#include <chrono>
#include "app_start_time_collection.h"

namespace OHOS::perftest {
    using namespace std;
    const string EVENT_DOMAIN = "PERFORMANCE";
    const string EVENT_NAME = "APP_START";
    const int32_t LISTEN_TIMEOUT = 10000;
    const int32_t LISTEN_TIMEOUT_UNIT = 500;
    const string RESPONSE_TIME_PARAM = "RESPONSE_LATENCY";
    const string COMPLETE_TIME_PARAM = "E2E_LATENCY";

    AppStartTimeCollection::AppStartTimeCollection(PerfMetric perfMetric) : DataCollection(perfMetric)
    {
        isCollecting_ = false;
        responseTime_ = INVALID_VALUE;
        completeTime_ = INVALID_VALUE;
    }

    void AppStartTimeCollection::StartCollection(ApiCallErr &error)
    {
        appStartListener_ = std::make_shared<TimeListener>(bundleName_);
        ListenerRule domainNameRule(EVENT_DOMAIN, EVENT_NAME, RuleType::WHOLE_WORD);
        vector<ListenerRule> sysRules;
        sysRules.push_back(domainNameRule);
        auto ret = HiSysEventManager::AddListener(appStartListener_, sysRules);
        if (ret != ZERO) {
            error = ApiCallErr(ERR_DATA_COLLECTION_FAILED, "Start app start measure failed");
        }
        isCollecting_ = true;
    }

    double AppStartTimeCollection::StopCollectionAndGetResult(ApiCallErr &error)
    {
        if (isCollecting_) {
            int32_t sleepTime = ZERO;
            do {
                this_thread::sleep_for(chrono::milliseconds(LISTEN_TIMEOUT_UNIT));
                sleepTime += LISTEN_TIMEOUT_UNIT;
            } while ((!appStartListener_->GetEvent()) && sleepTime < LISTEN_TIMEOUT);
            HiSysEventManager::RemoveListener(appStartListener_);
            if (!appStartListener_->GetEvent()) {
                LOG_E("Get event of app start failed");
                isCollecting_ = false;
                return INVALID_VALUE;
            }
            shared_ptr<HiviewDFX::HiSysEventRecord> eventRecord = appStartListener_->GetEvent();
            uint64_t responseRes;
            if (eventRecord->GetParamValue(RESPONSE_TIME_PARAM, responseRes) != ZERO) {
                LOG_E("Get the RESPONSE_LATENCY of HiSysEventRecord failed");
                isCollecting_ = false;
                return responseTime_;
            }
            responseTime_ = static_cast<double>(responseRes);
            uint64_t completeRes;
            if (eventRecord->GetParamValue(COMPLETE_TIME_PARAM, completeRes) != ZERO) {
                LOG_E("Get the E2E_LATENCY of HiSysEventRecord failed");
                isCollecting_ = false;
                return completeTime_;
            }
            completeTime_ = static_cast<double>(completeRes);
            isCollecting_ = false;
        }
        switch (perfMetric_) {
            case APP_START_RESPONSE_TIME:
                return responseTime_;
            case APP_START_COMPLETE_TIME:
                return completeTime_;
            default:
                return INVALID_VALUE;
        }
        return INVALID_VALUE;
    }
}
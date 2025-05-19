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
#include "page_switch_time_collection.h"

namespace OHOS::perftest {
    using namespace std;
    const string EVENT_DOMAIN = "PERFORMANCE";
    const string EVENT_NAME = "ABILITY_OR_PAGE_SWITCH";
    const int32_t LISTEN_TIMEOUT = 10000;
    const int32_t LISTEN_TIMEOUT_UNIT = 500;
    const string COMPLETE_TIME_PARAM = "E2E_LATENCY";

    PageSwitchTimeCollection::PageSwitchTimeCollection(PerfMetric perfMetric) : DataCollection(perfMetric)
    {
        completeTime_ = INVALID_VALUE;
    }

    void PageSwitchTimeCollection::StartCollection(ApiCallErr &error)
    {
        pageSwitchListener_ = std::make_shared<TimeListener>(bundleName_);
        ListenerRule domainNameRule(EVENT_DOMAIN, EVENT_NAME, RuleType::WHOLE_WORD);
        vector<ListenerRule> sysRules;
        sysRules.push_back(domainNameRule);
        auto ret = HiSysEventManager::AddListener(pageSwitchListener_, sysRules);
        if (ret != ZERO) {
            error = ApiCallErr(ERR_DATA_COLLECTION_FAILED, "Start page switch measure failed");
        }
    }

    double PageSwitchTimeCollection::StopCollectionAndGetResult(ApiCallErr &error)
    {
        int32_t sleepTime = ZERO;
        do {
            this_thread::sleep_for(chrono::milliseconds(LISTEN_TIMEOUT_UNIT));
            sleepTime += LISTEN_TIMEOUT_UNIT;
        } while (!pageSwitchListener_->GetEvent() && sleepTime < LISTEN_TIMEOUT);
        HiSysEventManager::RemoveListener(pageSwitchListener_);
        if (!pageSwitchListener_->GetEvent()) {
            LOG_E("Get event of page switch failed");
            return INVALID_VALUE;
        }
        shared_ptr<HiviewDFX::HiSysEventRecord> eventRecord = pageSwitchListener_->GetEvent();
        uint64_t res;
        if (eventRecord->GetParamValue(COMPLETE_TIME_PARAM, res) != ZERO) {
            LOG_E("Get the E2E_LATENCY of HiSysEventRecord failed");
            return completeTime_;
        }
        completeTime_ = static_cast<double>(res);
        return completeTime_;
    }
}
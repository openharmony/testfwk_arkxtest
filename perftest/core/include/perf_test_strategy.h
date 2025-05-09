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

#ifndef PERF_TEST_STRATEGY_H
#define PERF_TEST_STRATEGY_H

#include "frontend_api_handler.h"
#include "data_collection.h"
#include "duration_collection.h"
#include "cpu_collection.h"
#include "memory_collection.h"
#include "app_start_time_collection.h"
#include "page_switch_time_collection.h"
#include "list_swipe_fps_collection.h"

namespace OHOS::perftest {
    using namespace std;
    
    static const map<PerfMetric, function<shared_ptr<DataCollection>(PerfMetric)>> g_dataCollectionMap = {
        {DURATION, [](PerfMetric metric) { return make_shared<DurationCollection>(metric); }},
        {CPU_LOAD, [](PerfMetric metric) { return make_shared<CpuCollection>(metric); }},
        {CPU_USAGE, [](PerfMetric metric) { return make_shared<CpuCollection>(metric); }},
        {MEMORY_RSS, [](PerfMetric metric) { return make_shared<MemoryCollection>(metric); }},
        {MEMORY_PSS, [](PerfMetric metric) { return make_shared<MemoryCollection>(metric); }},
        {APP_START_RESPONSE_TIME, [](PerfMetric metric) { return make_shared<AppStartTimeCollection>(metric); }},
        {APP_START_COMPLETE_TIME, [](PerfMetric metric) { return make_shared<AppStartTimeCollection>(metric); }},
        {PAGE_SWITCH_COMPLETE_TIME, [](PerfMetric metric) { return make_shared<PageSwitchTimeCollection>(metric); }},
        {LIST_SWIPE_FPS, [](PerfMetric metric) { return make_shared<ListSwipeFpsCollection>(metric); }},
    };

    class PerfTestStrategy {
    public:
        explicit PerfTestStrategy(set<PerfMetric> metrics, string actionCodeRef, string resetCodeRef,
                                  string bundleName, int32_t iterations, int32_t timeout, ApiCallErr &error);
        ~PerfTestStrategy() {};
        set<PerfMetric> GetPerfMetrics();
        string GetActionCodeRef();
        string GetResetCodeRef();
        string GetBundleName();
        int32_t GetIterations();
        int32_t GetTimeout();
        map<PerfMetric, shared_ptr<DataCollection>> GetDataCollections();
        void GetBundleNameByPid(ApiCallErr &error);
        void CreateDataCollections();
    
    private:
        set<PerfMetric> perfMetrics_;
        map<PerfMetric, shared_ptr<DataCollection>> dataCollections_;
        string actionCodeRef_;
        string resetCodeRef_;
        string bundleName_;
        int32_t iterations_;
        int32_t timeout_;
    };
} // namespace OHOS::perftest

#endif
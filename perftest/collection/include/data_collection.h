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

#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

#include "common_utils.h"
#include "frontend_api_defines.h"
#include "hisysevent_manager.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace OHOS::HiviewDFX;
    static const double INITIAL_VALUE = 0.00;
    static const double INVALID_VALUE = -1.00;

    enum PerfMetric: int32_t {
        DURATION,
        CPU_LOAD,
        CPU_USAGE,
        MEMORY_RSS,
        MEMORY_PSS,
        APP_START_RESPONSE_TIME,
        APP_START_COMPLETE_TIME,
        PAGE_SWITCH_COMPLETE_TIME,
        LIST_SWIPE_FPS,
        METRIC_COUNT
    };

    class DataCollection {
    public:
        DataCollection(PerfMetric perfMetric) : perfMetric_(perfMetric) {};
        DataCollection() = default;
        virtual ~DataCollection() = default;
        virtual void StartCollection(ApiCallErr &error) = 0;
        virtual double StopCollectionAndGetResult(ApiCallErr &error) = 0;
        void SetPid(int32_t pid);
        void SetBundleName(string bundleName);
        int32_t GetPidByBundleName();
        bool IsProcessExist(int32_t pid);
        bool ExecuteCommand(const string command, string& result);
    protected:
        int32_t pid_;
        string bundleName_;
        PerfMetric perfMetric_;
    };

    class TimeListener : public HiviewDFX::HiSysEventListener {
    public:
        TimeListener(string bundleName) : bundleName_(bundleName) {}
        void OnEvent(shared_ptr<HiSysEventRecord> record);
        void OnServiceDied();
        shared_ptr<HiviewDFX::HiSysEventRecord> GetEvent();
    private:
        string bundleName_;
        shared_ptr<HiviewDFX::HiSysEventRecord> lastEvent_ = nullptr;
    };
} // namespace OHOS::perftest

#endif
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

#ifndef CPU_COLLECTION_H
#define CPU_COLLECTION_H

#include "data_collection.h"
#include "cpu_collector.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace HiviewDFX;
    using namespace HiviewDFX::UCollectUtil;

    class CpuCollection : public DataCollection {
    public:
        CpuCollection(PerfMetric perfMetric) : DataCollection(perfMetric) {};
        CpuCollection() = default;
        ~CpuCollection() = default;
        void StartCollection(ApiCallErr &error) override;
        double StopCollectionAndGetResult(ApiCallErr &error) override;
    private:
        std::shared_ptr<CpuCollector> cpuCollector_;
        inline static bool isCollecting_ = false;
        inline static double cpuLoad_ = INVALID_VALUE;
        inline static double cpuUsage_ = INVALID_VALUE;
    };
} // namespace OHOS::perftest

#endif
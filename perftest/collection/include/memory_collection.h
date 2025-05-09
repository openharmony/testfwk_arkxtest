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

#ifndef MEMORY_COLLECTION_H
#define MEMORY_COLLECTION_H

#include "data_collection.h"

namespace OHOS::perftest {
    using namespace std;

    class MemoryCollection : public DataCollection {
    public:
        MemoryCollection(PerfMetric perfMetric) : DataCollection(perfMetric) {};
        MemoryCollection() = default;
        ~MemoryCollection() = default;
        void StartCollection(ApiCallErr &error) override;
        double StopCollectionAndGetResult(ApiCallErr &error) override;
    private:
        inline static bool isCollecting_ = false;
        inline static double startRss_ = INITIAL_VALUE;
        inline static double endRss_ = INITIAL_VALUE;
        inline static double startPss_ = INITIAL_VALUE;
        inline static double endPss_ = INITIAL_VALUE;
        inline static double memoryRss_ = INVALID_VALUE;
        inline static double memoryPss_ = INVALID_VALUE;
    };
} // namespace OHOS::perftest

#endif
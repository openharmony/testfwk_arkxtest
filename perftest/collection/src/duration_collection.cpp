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

#include <chrono>
#include "duration_collection.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace std::chrono;

    void DurationCollection::StartCollection(ApiCallErr &error)
    {
        startMs_ = static_cast<double>(time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count());
        LOG_I("Start collect duration, startMS = %{public}.2f", startMs_);
    }

    double DurationCollection::StopCollectionAndGetResult(ApiCallErr &error)
    {
        endMs_ = static_cast<double>(time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count());
        LOG_I("End collect duration, endMs = %{public}.2f", endMs_);
        duration_ = endMs_ - startMs_;
        return duration_;
    }
} // namespace OHOS::perftest

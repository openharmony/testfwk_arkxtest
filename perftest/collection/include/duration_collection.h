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

#ifndef DURATION_COLLECTION_H
#define DURATION_COLLECTION_H

#include "data_collection.h"

namespace OHOS::perftest {
    using namespace std;

    class DurationCollection : public DataCollection {
    public:
        using DataCollection::DataCollection;
        DurationCollection() = default;
        ~DurationCollection() = default;
        void StartCollection(ApiCallErr &error) override;
        double StopCollectionAndGetResult(ApiCallErr &error) override;
    private:
        double startMs_ = INITIAL_VALUE;
        double endMs_ = INITIAL_VALUE;
        double duration_ = INVALID_VALUE;
    };
} // namespace OHOS::perftest

#endif
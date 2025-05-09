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

#ifndef PERF_TEST_H
#define PERF_TEST_H

#include "frontend_api_handler.h"
#include "perf_test_strategy.h"
#include "duration_collection.h"

namespace OHOS::perftest {
    using namespace std;

    class PerfTestCallback {
    public:
        PerfTestCallback() = default;
        virtual ~PerfTestCallback() = default;
        virtual void OnCall(const std::string&& codeRef, const int32_t timeout, ApiCallErr &error) {};
        virtual void OnDestory(const list<std::string> codeRefs, ApiCallErr &error) {};
    };

    class PerfTest : public BackendClass {
    public:
        explicit PerfTest(unique_ptr<PerfTestStrategy> perfTestStrategy, unique_ptr<PerfTestCallback> perfTestCallback)
            : perfTestStrategy_(move(perfTestStrategy)), perfTestCallback_(move(perfTestCallback)) {};
        ~PerfTest() override {};

        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return PERF_TEST_DEF;
        }

        int32_t GetPidByBundleName(string bundleName);
        void RunTest(ApiCallErr &error);
        nlohmann::json GetMeasureResult(PerfMetric perfMetric, ApiCallErr &error);
        bool IsMeasureRunning();
        void Destory(ApiCallErr &error);
    private:
        unique_ptr<PerfTestStrategy> perfTestStrategy_;
        unique_ptr<PerfTestCallback> perfTestCallback_;
        map<PerfMetric, list<double>> measureResult_;
        bool isMeasureComplete_ = false;
        bool isMeasureRunning_ = false;
    };
} // namespace OHOS::perftest

#endif
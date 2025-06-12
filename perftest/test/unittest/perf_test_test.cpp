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

#include "gtest/gtest.h"
#define private public
#define protected public
#include "perf_test.h"
#include "perf_test_strategy.h"
#undef private

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::perftest;

class PerfTestTest : public testing::Test {
public:
    ~PerfTestTest() override = default;
protected:
    unique_ptr<PerfTest> perfTest_;
    void SetUp()
    {
        set<PerfMetric> metrics;
        metrics.insert(PerfMetric::DURATION);
        ApiCallErr exception = ApiCallErr(NO_ERROR);
        int32_t iterations = 10;
        int32_t timeout = 10000;
        unique_ptr<PerfTestStrategy> perfTestStrategy = make_unique<PerfTestStrategy>(metrics, "callback#1", "",
                                                        "com.unittest.test", iterations, timeout, exception);
        unique_ptr<PerfTestCallback> perfTestCallback = make_unique<PerfTestCallback>();
        perfTest_ = make_unique<PerfTest>(move(perfTestStrategy), move(perfTestCallback));
    }
};

HWTEST_F(PerfTestTest, testPerfTestStrategy, TestSize.Level1)
{
    set<PerfMetric> metrics = perfTest_->perfTestStrategy_->GetPerfMetrics();
    ASSERT_EQ(metrics.size(), 1);
    ASSERT_EQ(perfTest_->perfTestStrategy_->GetActionCodeRef(), "callback#1");
    ASSERT_EQ(perfTest_->perfTestStrategy_->GetResetCodeRef(), "");
    ASSERT_EQ(perfTest_->perfTestStrategy_->GetBundleName(), "com.unittest.test");
    ASSERT_EQ(perfTest_->perfTestStrategy_->GetIterations(), 10);
    ASSERT_EQ(perfTest_->perfTestStrategy_->GetTimeout(), 10000);
}

HWTEST_F(PerfTestTest, testGetBundleNameByPid, TestSize.Level1)
{
    perfTest_->perfTestStrategy_->bundleName_ = "";
    set<PerfMetric> metrics = perfTest_->perfTestStrategy_->GetPerfMetrics();
    ApiCallErr error = ApiCallErr(NO_ERROR);
    perfTest_->perfTestStrategy_->GetBundleNameByPid(error);
    ASSERT_EQ(error.code_, NO_ERROR);
    ASSERT_TRUE(perfTest_->perfTestStrategy_->bundleName_.find("perftest_unittest") != string::npos);
}

HWTEST_F(PerfTestTest, testCreateDataCollections, TestSize.Level1)
{
    set<PerfMetric> metrics;
    metrics.insert(PerfMetric::DURATION);
    metrics.insert(PerfMetric::CPU_LOAD);
    metrics.insert(PerfMetric::CPU_USAGE);
    metrics.insert(PerfMetric::MEMORY_RSS);
    metrics.insert(PerfMetric::MEMORY_PSS);
    metrics.insert(PerfMetric::APP_START_RESPONSE_TIME);
    metrics.insert(PerfMetric::APP_START_COMPLETE_TIME);
    metrics.insert(PerfMetric::PAGE_SWITCH_COMPLETE_TIME);
    metrics.insert(PerfMetric::LIST_SWIPE_FPS);
    perfTest_->perfTestStrategy_->perfMetrics_ = metrics;
    perfTest_->perfTestStrategy_->CreateDataCollections();
    ASSERT_EQ(perfTest_->perfTestStrategy_->dataCollections_.size(), 9);
}

HWTEST_F(PerfTestTest, testGetPidByBundleName, TestSize.Level1)
{
    int32_t currentPid = getpid();
    int32_t pid = perfTest_->GetPidByBundleName("perftest_unittest");
    ASSERT_EQ(currentPid, pid);
}

HWTEST_F(PerfTestTest, testRunTest, TestSize.Level1)
{
    set<PerfMetric> metrics;
    metrics.insert(PerfMetric::DURATION);
    perfTest_->perfTestStrategy_->perfMetrics_ = metrics;
    ApiCallErr error = ApiCallErr(NO_ERROR);
    perfTest_->RunTest(error);
    ASSERT_EQ(error.code_, NO_ERROR);
    ASSERT_EQ(perfTest_->isMeasureComplete_, true);
    ASSERT_EQ(perfTest_->IsMeasureRunning(), false);
}

HWTEST_F(PerfTestTest, testGetMeasureResult, TestSize.Level1)
{
    set<PerfMetric> metrics;
    perfTest_->perfTestStrategy_->perfMetrics_ = metrics;
    ApiCallErr error1 = ApiCallErr(NO_ERROR);
    perfTest_->GetMeasureResult(PerfMetric::DURATION, error1);
    ASSERT_EQ(error1.code_, ERR_INVALID_INPUT);
    metrics.insert(PerfMetric::DURATION);
    perfTest_->perfTestStrategy_->perfMetrics_ = metrics;
    ApiCallErr error2 = ApiCallErr(NO_ERROR);
    perfTest_->GetMeasureResult(PerfMetric::DURATION, error2);
    ASSERT_EQ(error2.code_, ERR_GET_RESULT_FAILED);
    perfTest_->measureResult_[PerfMetric::DURATION].push_back(10);
    perfTest_->measureResult_[PerfMetric::DURATION].push_back(20);
    perfTest_->measureResult_[PerfMetric::DURATION].push_back(30);
    perfTest_->perfTestStrategy_->iterations_ = 3;
    perfTest_->isMeasureComplete_ = true;
    ApiCallErr error3 = ApiCallErr(NO_ERROR);
    nlohmann::json res = perfTest_->GetMeasureResult(PerfMetric::DURATION, error3);
    ASSERT_EQ(error3.code_, NO_ERROR);
    ASSERT_EQ(res["maximum"], 30);
    ASSERT_EQ(res["minimum"], 10);
    ASSERT_EQ(res["average"], 20);
}

HWTEST_F(PerfTestTest, testDestroy, TestSize.Level1)
{
    ApiCallErr error = ApiCallErr(NO_ERROR);
    perfTest_->Destroy(error);
    ASSERT_EQ(error.code_, NO_ERROR);
}

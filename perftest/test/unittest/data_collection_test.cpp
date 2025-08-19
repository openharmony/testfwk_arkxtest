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

#include <unistd.h>
#include <thread>
#include <chrono>
#include "gtest/gtest.h"
#define private public
#define protected public
#include "duration_collection.h"
#include "cpu_collection.h"
#include "memory_collection.h"
#include "list_swipe_fps_collection.h"
#include "app_start_time_collection.h"
#include "page_switch_time_collection.h"
#undef private

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::perftest;

class DataCollectionTest : public testing::Test {
public:
    ~DataCollectionTest() override = default;
protected:
    shared_ptr<DataCollection> dataCollection_ = make_shared<DurationCollection>(PerfMetric::DURATION);
};

HWTEST_F(DataCollectionTest, testGetPidByBundleName, TestSize.Level1)
{
    dataCollection_->SetBundleName("com.unittest.test");
    EXPECT_EQ(dataCollection_->GetPidByBundleName(), -1);
}

HWTEST_F(DataCollectionTest, testIsProcessExist, TestSize.Level1)
{
    int32_t currentPid = getpid();
    EXPECT_EQ(dataCollection_->IsProcessExist(currentPid), true);
    int32_t errorPid = 100000;
    EXPECT_EQ(dataCollection_->IsProcessExist(errorPid), false);
}

HWTEST_F(DataCollectionTest, testExecuteCommand, TestSize.Level1)
{
    string command = "ls";
    string result;
    EXPECT_EQ(dataCollection_->ExecuteCommand(command, result), true);
    EXPECT_NE(result, "");
}

HWTEST_F(DataCollectionTest, testStartCollection, TestSize.Level1)
{
    ApiCallErr error = ApiCallErr(NO_ERROR);
    dataCollection_->StartCollection(error);
    EXPECT_EQ(error.code_, NO_ERROR);
}

HWTEST_F(DataCollectionTest, testStopCollectionAndGetResult, TestSize.Level1)
{
    ApiCallErr error = ApiCallErr(NO_ERROR);
    dataCollection_->StartCollection(error);
    EXPECT_EQ(error.code_, NO_ERROR);
    this_thread::sleep_for(chrono::milliseconds(1000));
    double duration = dataCollection_->StopCollectionAndGetResult(error);
    EXPECT_EQ(error.code_, NO_ERROR);
    EXPECT_GT(duration, 0);
}

HWTEST_F(DataCollectionTest, testStartCollectionWithWrongBundleName, TestSize.Level1)
{
    shared_ptr<DataCollection> cpuCollection = make_shared<CpuCollection>(PerfMetric::CPU_USAGE);
    ApiCallErr error1 = ApiCallErr(NO_ERROR);
    cpuCollection->StartCollection(error1);
    EXPECT_EQ(error1.code_, ERR_DATA_COLLECTION_FAILED);
    shared_ptr<DataCollection> memoryCollection = make_shared<MemoryCollection>(PerfMetric::MEMORY_RSS);
    ApiCallErr error2 = ApiCallErr(NO_ERROR);
    cpuCollection->StartCollection(error2);
    EXPECT_EQ(error2.code_, ERR_DATA_COLLECTION_FAILED);
}

HWTEST_F(DataCollectionTest, testCpuStopCollectionAndGetResult, TestSize.Level1)
{
    shared_ptr<CpuCollection> cpuCollection1 = make_shared<CpuCollection>(PerfMetric::CPU_LOAD);
    shared_ptr<CpuCollection> cpuCollection2 = make_shared<CpuCollection>(PerfMetric::CPU_USAGE);
    shared_ptr<CpuCollection> cpuCollection3 = make_shared<CpuCollection>(PerfMetric::DURATION);
    cpuCollection1->cpuLoad_ = 1.00;
    cpuCollection1->cpuUsage_ = 2.00;
    ApiCallErr error = ApiCallErr(NO_ERROR);
    EXPECT_EQ(cpuCollection1->StopCollectionAndGetResult(error), 1.00);
    EXPECT_EQ(cpuCollection2->StopCollectionAndGetResult(error), 2.00);
    EXPECT_EQ(cpuCollection3->StopCollectionAndGetResult(error), INVALID_VALUE);
}

HWTEST_F(DataCollectionTest, testMemoryStopCollectionAndGetResult, TestSize.Level1)
{
    shared_ptr<MemoryCollection> memoryCollection1 = make_shared<MemoryCollection>(PerfMetric::MEMORY_RSS);
    shared_ptr<MemoryCollection> memoryCollection2 = make_shared<MemoryCollection>(PerfMetric::MEMORY_PSS);
    shared_ptr<MemoryCollection> memoryCollection3 = make_shared<MemoryCollection>(PerfMetric::DURATION);
    memoryCollection1->memoryRss_ = 1.00;
    memoryCollection1->memoryPss_ = 2.00;
    ApiCallErr error = ApiCallErr(NO_ERROR);
    EXPECT_EQ(memoryCollection1->StopCollectionAndGetResult(error), 1.00);
    EXPECT_EQ(memoryCollection2->StopCollectionAndGetResult(error), 2.00);
    EXPECT_EQ(memoryCollection3->StopCollectionAndGetResult(error), INVALID_VALUE);
}

HWTEST_F(DataCollectionTest, testGetSwipeState, TestSize.Level1)
{
    shared_ptr<ListSwipeFpsCollection> fpsCollection = make_shared<ListSwipeFpsCollection>(PerfMetric::LIST_SWIPE_FPS);
    fpsCollection->pid_ = 0;
    string line1 = "bundlename-0 ( 0000) [000] .....  123.45678: trace_tag: S|0|H:APP_LIST_FLING|0|M0000";
    string traceTag = "H:APP_LIST_FLING";
    const vector<string> tokens1 = fpsCollection->SplitString(line1, ' ');
    int32_t stateIndex1 = 0;
    EXPECT_EQ(fpsCollection->GetSwipeState(tokens1, traceTag, stateIndex1), "SWIPE_START");
    EXPECT_EQ(stateIndex1, 7);
    EXPECT_EQ(fpsCollection->GetTime(tokens1, stateIndex1 - 2), 123.45678);
    string line2 = "bundlename-0 (0000) [000] .....  123.45678: trace_tag: F|0|H:APP_LIST_FLING|0|M0000";
    const vector<string> tokens2 = fpsCollection->SplitString(line2, ' ');
    int32_t stateIndex2 = 0;
    EXPECT_EQ(fpsCollection->GetSwipeState(tokens2, traceTag, stateIndex2), "SWIPE_FINISH");
    EXPECT_EQ(stateIndex2, 6);
    EXPECT_EQ(fpsCollection->GetTime(tokens2, stateIndex2 - 2), 123.45678);
}

HWTEST_F(DataCollectionTest, testGetTime, TestSize.Level1)
{
    shared_ptr<ListSwipeFpsCollection> fpsCollection = make_shared<ListSwipeFpsCollection>(PerfMetric::LIST_SWIPE_FPS);
    string line = "bundlename-pid ( 0000) [000] .....  123.45678: trace_tag: S|1111|H:APP_LIST_FLING|0|M0000";
    const vector<string> tokens = fpsCollection->SplitString(line, ' ');
    EXPECT_EQ(fpsCollection->GetTime(tokens, 5), 123.45678);
}

HWTEST_F(DataCollectionTest, testSplitString, TestSize.Level1)
{
    shared_ptr<ListSwipeFpsCollection> fpsCollection = make_shared<ListSwipeFpsCollection>(PerfMetric::LIST_SWIPE_FPS);
    string str1 = "aaa bbb ccc";
    vector<string> tokens1 = fpsCollection->SplitString(str1, ' ');
    EXPECT_EQ(tokens1[0], "aaa");
    EXPECT_EQ(tokens1[1], "bbb");
    EXPECT_EQ(tokens1[2], "ccc");
    string str2 = "aaa,,bbb,,,ccc";
    vector<string> tokens2 = fpsCollection->SplitString(str2, ',');
    EXPECT_EQ(tokens2[0], "aaa");
    EXPECT_EQ(tokens2[1], "bbb");
    EXPECT_EQ(tokens2[2], "ccc");
}

HWTEST_F(DataCollectionTest, testAppStartCollectionAndGetResult, TestSize.Level1)
{
    nlohmann::json recordJson;
    recordJson["BUNDLE_NAME"] = "com.unittest.test";
    recordJson["RESPONSE_LATENCY"] = 50;
    recordJson["E2E_LATENCY"] = 100;
    shared_ptr<HiSysEventRecord> record = make_shared<HiSysEventRecord>(recordJson.dump());
    shared_ptr<AppStartTimeCollection> appStartCollection1 =
                                            make_shared<AppStartTimeCollection>(PerfMetric::APP_START_RESPONSE_TIME);
    shared_ptr<AppStartTimeCollection> appStartCollection2 =
                                            make_shared<AppStartTimeCollection>(PerfMetric::APP_START_COMPLETE_TIME);
    ApiCallErr error = ApiCallErr(NO_ERROR);
    appStartCollection1->bundleName_ = "com.unittest.test";
    appStartCollection1->appStartListener_ = std::make_shared<TimeListener>(appStartCollection1->bundleName_);
    appStartCollection1->appStartListener_->OnEvent(record);
    EXPECT_NE(appStartCollection1->appStartListener_->GetEvent(), nullptr);
    appStartCollection1->isCollecting_ = true;
    EXPECT_EQ(appStartCollection1->StopCollectionAndGetResult(error), 50);
    EXPECT_EQ(appStartCollection2->StopCollectionAndGetResult(error), 100);
}

HWTEST_F(DataCollectionTest, testPageSwitchCollectionAndGetResult, TestSize.Level1)
{
    nlohmann::json recordJson;
    recordJson["BUNDLE_NAME"] = "com.unittest.test";
    recordJson["E2E_LATENCY"] = 100;
    shared_ptr<HiSysEventRecord> record = make_shared<HiSysEventRecord>(recordJson.dump());
    shared_ptr<PageSwitchTimeCollection> pageSwitchCollection =
                                        make_shared<PageSwitchTimeCollection>(PerfMetric::PAGE_SWITCH_COMPLETE_TIME);
    ApiCallErr error = ApiCallErr(NO_ERROR);
    pageSwitchCollection->bundleName_ = "com.unittest.test";
    pageSwitchCollection->pageSwitchListener_ = std::make_shared<TimeListener>(pageSwitchCollection->bundleName_);
    pageSwitchCollection->pageSwitchListener_->OnEvent(record);
    EXPECT_NE(pageSwitchCollection->pageSwitchListener_->GetEvent(), nullptr);
    EXPECT_EQ(pageSwitchCollection->StopCollectionAndGetResult(error), 100);
}
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

#include <fstream>
#include <sstream>
#include <set>
#include "perf_test.h"
#include "ipc_skeleton.h"

namespace OHOS::perftest {
    using namespace std;
    
    int32_t PerfTest::GetPidByBundleName(string bundleName)
    {
        string getPidCmd = "pidof " + bundleName;
        int32_t pid = -1;
        FILE *fp = popen(getPidCmd.c_str(), "r");
        if (fp == nullptr) {
            LOG_E("Excute popen to get process pid failed");
            return pid;
        }
        string pidStr;
        char bufferInfo[1024] = { '\0' };
        while (fgets(bufferInfo, sizeof(bufferInfo), fp) != nullptr) {
            pidStr += bufferInfo;
        }
        pclose(fp);
        LOG_I("Pid of process %{public}s is %{public}s", bundleName.c_str(), pidStr.c_str());
        stringstream pidStream(pidStr);
        pidStream >> pid;
        return pid;
    }

    void PerfTest::RunTest(ApiCallErr &error)
    {
        LOG_I("%{public}s called", __func__);
        auto bundleName = perfTestStrategy_->GetBundleName();
        auto dataCollections = perfTestStrategy_->GetDataCollections();
        for (auto dataCollection : dataCollections) {
            dataCollection.second->SetBundleName(bundleName);
        }
        isMeasureRunning_ = true;
        measureResult_.clear();
        for (int32_t iteration = 0; iteration < perfTestStrategy_->GetIterations(); iteration++) {
            for (auto& dataCollection : dataCollections) {
                dataCollection.second->StartCollection(error);
                if (error.code_ != NO_ERROR) {
                    isMeasureRunning_ = false;
                    return;
                }
            }
            perfTestCallback_->OnCall(perfTestStrategy_->GetActionCodeRef(), perfTestStrategy_->GetTimeout(), error);
            if (error.code_ != NO_ERROR) {
                LOG_E("ActionCode call error");
                isMeasureRunning_ = false;
                return;
            }
            for (auto& dataCollection : dataCollections) {
                double res = dataCollection.second->StopCollectionAndGetResult(error);
                if (error.code_ != NO_ERROR) {
                    isMeasureRunning_ = false;
                    return;
                }
                measureResult_[dataCollection.first].push_back(res);
            }
            perfTestCallback_->OnCall(perfTestStrategy_->GetResetCodeRef(), perfTestStrategy_->GetTimeout(), error);
            if (error.code_ != NO_ERROR) {
                LOG_E("ResetCode call error");
                isMeasureRunning_ = false;
                return;
            }
        }
        isMeasureRunning_ = false;
        isMeasureComplete_ = true;
    }

    nlohmann::json PerfTest::GetMeasureResult(PerfMetric perfMetric, ApiCallErr &error)
    {
        LOG_I("%{public}s called", __func__);
        nlohmann::json resData;
        if (perfTestStrategy_->GetPerfMetrics().count(perfMetric) == ZERO) {
            error = ApiCallErr(ERR_INVALID_INPUT, "PerfMetric: " + to_string(perfMetric) + " is not set to measure");
            return resData;
        }
        if (!isMeasureComplete_ || measureResult_.count(perfMetric) == ZERO ||
            measureResult_[perfMetric].size() != perfTestStrategy_->GetIterations()) {
            error = ApiCallErr(ERR_GET_RESULT_FAILED,
                               "PerfMetric: " + to_string(perfMetric) + " has not been measured yet");
            return resData;
        }
        list<double> resList = measureResult_[perfMetric];
        list<double> validResList;
        for (auto res : resList) {
            if (res > INVALID_VALUE) {
                validResList.push_back(res);
            }
        }
        resData["metric"] = perfMetric;
        resData["roundValues"] = resList;
        if (validResList.empty()) {
            resData["maximum"] = INITIAL_VALUE;
            resData["minimum"] = INITIAL_VALUE;
            resData["average"] = INITIAL_VALUE;
        } else {
            resData["maximum"] = *max_element(validResList.begin(), validResList.end());
            resData["minimum"] = *min_element(validResList.begin(), validResList.end());
            resData["average"] = accumulate(validResList.begin(), validResList.end(), 0.0) / validResList.size();
        }
        return resData;
    }

    bool PerfTest::IsMeasureRunning()
    {
        return isMeasureRunning_;
    }

    void PerfTest::Destroy(ApiCallErr &error)
    {
        list<string> codeRefs;
        codeRefs.push_back(perfTestStrategy_->GetActionCodeRef());
        if (!perfTestStrategy_->GetResetCodeRef().empty()) {
            codeRefs.push_back(perfTestStrategy_->GetResetCodeRef());
        }
        perfTestCallback_->OnDestroy(codeRefs, error);
    }
} // namespace OHOS::perftest

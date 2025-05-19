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
#include "perf_test_strategy.h"
#include "ipc_skeleton.h"

namespace OHOS::perftest {
    using namespace std;

    PerfTestStrategy::PerfTestStrategy(set<PerfMetric> metrics, string actionCodeRef, string resetCodeRef,
                                       string bundleName, int32_t iterations, int32_t timeout, ApiCallErr &error)
    {
        perfMetrics_ = metrics;
        actionCodeRef_ = actionCodeRef;
        resetCodeRef_ = resetCodeRef;
        bundleName_ = bundleName;
        iterations_ = iterations;
        timeout_ = timeout;
        GetBundleNameByPid(error);
        CreateDataCollections();
    }

    set<PerfMetric> PerfTestStrategy::GetPerfMetrics()
    {
        return perfMetrics_;
    }

    string PerfTestStrategy::GetActionCodeRef()
    {
        return actionCodeRef_;
    }
    
    string PerfTestStrategy::GetResetCodeRef()
    {
        return resetCodeRef_;
    }
    
    string PerfTestStrategy::GetBundleName()
    {
        return bundleName_;
    }
    
    int32_t PerfTestStrategy::GetIterations()
    {
        return iterations_;
    }
    
    int32_t PerfTestStrategy::GetTimeout()
    {
        return timeout_;
    }

    map<PerfMetric, shared_ptr<DataCollection>> PerfTestStrategy::GetDataCollections()
    {
        return dataCollections_;
    }

    void PerfTestStrategy::GetBundleNameByPid(ApiCallErr &error)
    {
        if (bundleName_ != "") {
            return;
        }
        int32_t callingPid = IPCSkeleton::GetCallingPid();
        string filePath = "/proc/" + to_string(callingPid) + "/cmdline";
        ifstream inFile(filePath.c_str());
        if (!inFile) {
            LOG_E("Get bundleName by pid failed");
            error = ApiCallErr(ERR_INITIALIZE_FAILED, "Get current application bundleName failed");
            return;
        }
        getline(inFile, bundleName_);
        inFile.close();
        LOG_I("GetBundleNameByPid, pid: %{public}d, bundleName: %{public}s", callingPid, bundleName_.c_str());
    }

    void PerfTestStrategy::CreateDataCollections()
    {
        dataCollections_.clear();
        for (auto perfMetric : perfMetrics_) {
            if (g_dataCollectionMap.find(perfMetric) == g_dataCollectionMap.end()) {
                LOG_W("The DataCollection of PerfMetric::%{public}d is not existed", perfMetric);
                continue;
            }
            shared_ptr<DataCollection> dataCollection = g_dataCollectionMap.at(perfMetric)(perfMetric);
            if (dataCollection == nullptr) {
                LOG_W("Get DataCollection of PerfMetric::%{public}d failed", perfMetric);
                continue;
            }
            dataCollection -> SetBundleName(bundleName_);
            dataCollections_[perfMetric] = dataCollection;
        }
    }
} // namespace OHOS::perftest

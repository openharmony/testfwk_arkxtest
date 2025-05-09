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

#include "cpu_collection.h"
#include "test_server_client.h"
#include "test_server_error_code.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace OHOS::testserver;

    void CpuCollection::StartCollection(ApiCallErr &error)
    {
        if (isCollecting_) {
            LOG_I("Cpu collection has started");
            return;
        }
        pid_ = GetPidByBundleName(bundleName_);
        if (pid_ == -1) {
            error = ApiCallErr(ERR_INTERNAL, "The process does not exist during cpu collection");
            return;
        }
        ProcessCpuInfo processCpuInfo;
        if (TestServerClient::GetInstance().CollectProcessCpu(pid_, true, processCpuInfo) != TEST_SERVER_OK) {
            error = ApiCallErr(ERR_INTERNAL, "Start cpu collection failed");
            return;
        }
        isCollecting_ = true;
    }

    double CpuCollection::StopCollectionAndGetResult(ApiCallErr &error)
    {
        if (isCollecting_) {
            LOG_I("Stop cpu collection");
            if (!IsProcessExist(pid_)) {
                error = ApiCallErr(ERR_INTERNAL, "The process does not exist during cpu collection");
                isCollecting_ = false;
                return INVALID_VALUE;
            }
            ProcessCpuInfo processCpuInfo;
            if (TestServerClient::GetInstance().CollectProcessCpu(pid_, true, processCpuInfo) != TEST_SERVER_OK) {
                error = ApiCallErr(ERR_INTERNAL, "Stop cpu collection failed");
                isCollecting_ = false;
                return INVALID_VALUE;
            }
            cpuLoad_ = processCpuInfo.cpuLoad * ONE_HUNDRED;
            cpuUsage_ = processCpuInfo.cpuUsage * ONE_HUNDRED;
            LOG_I("End collect cpu, cpuLoad_ = %{public}.2f, cpuUsage_ = %{public}.2f", cpuLoad_, cpuUsage_);
            isCollecting_ = false;
        }
        switch (perfMetric_) {
            case CPU_LOAD:
                return cpuLoad_;
            case CPU_USAGE:
                return cpuUsage_;
            default:
                return INVALID_VALUE;
        }
        return INVALID_VALUE;
    }
} // namespace OHOS::perftest

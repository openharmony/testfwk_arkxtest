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

#include "memory_collection.h"
#include "test_server_client.h"
#include "test_server_error_code.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace testserver;

    void MemoryCollection::StartCollection(ApiCallErr &error)
    {
        if (isCollecting_) {
            LOG_I("Memory collection has started");
            return;
        }
        pid_ = GetPidByBundleName(bundleName_);
        if (pid_ == -1) {
            error = ApiCallErr(ERR_INTERNAL, "The process does not exist during memory collection");
            return;
        }
        ProcessMemoryInfo processMemoryInfo;
        if (TestServerClient::GetInstance().CollectProcessMemory(pid_, processMemoryInfo) != TEST_SERVER_OK) {
            error = ApiCallErr(ERR_INTERNAL, "Start memory collection failed");
            return;
        }
        startRss_ = static_cast<double>(processMemoryInfo.rss);
        startPss_ = static_cast<double>(processMemoryInfo.pss);
        LOG_I("Start collect memory, startRss_ = %{public}.2f, startPss_ = %{public}.2f", startRss_, startPss_);
        isCollecting_ = true;
    }

    double MemoryCollection::StopCollectionAndGetResult(ApiCallErr &error)
    {
        if (isCollecting_) {
            LOG_I("Stop memory collection");
            if (!IsProcessExist(pid_)) {
                error = ApiCallErr(ERR_INTERNAL, "The process does not exist during memory collection");
                return INVALID_VALUE;
            }
            ProcessMemoryInfo processMemoryInfo;
            if (TestServerClient::GetInstance().CollectProcessMemory(pid_, processMemoryInfo) != TEST_SERVER_OK) {
                error = ApiCallErr(ERR_INTERNAL, "Stop memory collection failed");
                isCollecting_ = false;
                return INVALID_VALUE;
            }
            endRss_ = static_cast<double>(processMemoryInfo.rss);
            endPss_ = static_cast<double>(processMemoryInfo.pss);
            LOG_I("End collect memory, endRss_ = %{public}.2f, endPss_ = %{public}.2f", endRss_, endPss_);
            memoryRss_ = endRss_;
            memoryPss_ = endPss_;
            isCollecting_ = false;
        }
        switch (perfMetric_) {
            case MEMORY_RSS:
                return memoryRss_;
            case MEMORY_PSS:
                return memoryPss_;
            default:
                return INVALID_VALUE;
        }
        return INVALID_VALUE;
    }
} // namespace OHOS::perftest

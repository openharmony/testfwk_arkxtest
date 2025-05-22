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

#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include "list_swipe_fps_collection.h"

namespace OHOS::perftest {
    using namespace std;
    const string TRACE_FILE_NAME = "/data/local/tmp/perftest_fps.ftrace";
    const string HITRACE_START_COMMAND = "hitrace -b 204800 --overwrite --trace_begin graphic nweb ace";
    const string HITRACE_FINISH_COMMAND = "hitrace --trace_finish -o " + TRACE_FILE_NAME;
    const string REMOVE_TRACE_FILE = "rm -f " + TRACE_FILE_NAME;
    const string SWIPE_START_STATE = "SWIPE_START";
    const string SWIPE_FINISH_STATE = "SWIPE_FINISH";
    const string SWIPE_INVALID_STATE = "SWIPE_INVALID";
    const string DO_COMPOSITION_TAG = "H:RSMainThread::DoComposition";
    const string APP_LIST_FLING_TAG = "H:APP_LIST_FLING";
    const string WEB_LIST_FLING_TAG = "H:WEB_LIST_FLING";
    const string APP_SWIPER_SCROLL_TAG = "H:APP_SWIPER_SCROLL";
    const string APP_SWIPER_FLING_TAG = "H:APP_SWIPER_FLING";

    void ListSwipeFpsCollection::StartCollection(ApiCallErr &error)
    {
        string executeRes;
        ExecuteCommand(HITRACE_FINISH_COMMAND, executeRes);
        ExecuteCommand(REMOVE_TRACE_FILE, executeRes);
        if (!ExecuteCommand(HITRACE_START_COMMAND, executeRes)) {
            error = ApiCallErr(ERR_DATA_COLLECTION_FAILED, "Start list swipe fps measure failed");
            return;
        }
        if (executeRes.find("OpenRecording done.") == string::npos) {
            error = ApiCallErr(ERR_DATA_COLLECTION_FAILED, "Start list swipe fps measure failed");
        }
    }

    double ListSwipeFpsCollection::StopCollectionAndGetResult(ApiCallErr &error)
    {
        string executeRes;
        if (!ExecuteCommand(HITRACE_FINISH_COMMAND, executeRes)) {
            error = ApiCallErr(ERR_DATA_COLLECTION_FAILED, "Stop list swipe fps measure failed");
            return INVALID_VALUE;
        }
        if (executeRes.find("Trace Closed.") == string::npos) {
            error = ApiCallErr(ERR_DATA_COLLECTION_FAILED, "Stop list swipe fps measure failed");
            return INVALID_VALUE;
        }
        pid_ = GetPidByBundleName(bundleName_);
        if (pid_ == -1) {
            error = ApiCallErr(ERR_DATA_COLLECTION_FAILED, "The process does not exist when fps trace parse");
            return INVALID_VALUE;
        }
        return ParseFpsByTrace(error);
    }
    
    double ListSwipeFpsCollection::ParseFpsByTrace(ApiCallErr &error)
    {
        ifstream file(TRACE_FILE_NAME);
        if (!file.is_open()) {
            error = ApiCallErr(ERR_DATA_COLLECTION_FAILED, "Get list swipe trace failed");
            return INVALID_VALUE;
        }
        double startTime = INITIAL_VALUE;
        double endTime = INITIAL_VALUE;
        int32_t frameCount = ZERO;
        string line;
        while (getline(file, line)) {
            vector<string> tokens = SplitString(line, ' ');
            if (line.find(APP_LIST_FLING_TAG) != string::npos) {
                if (GetSwipeState(tokens[INDEX_SEVEN], APP_LIST_FLING_TAG) == SWIPE_START_STATE) {
                    LOG_D("App list fling start");
                    startTime = GetTime(tokens[INDEX_FIVE]);
                } else if (GetSwipeState(tokens[INDEX_SEVEN], APP_LIST_FLING_TAG) == SWIPE_FINISH_STATE) {
                    LOG_D("App list fling finish");
                    endTime = GetTime(tokens[INDEX_FIVE]);
                }
            } else if (line.find(WEB_LIST_FLING_TAG) != string::npos) {
                if (GetSwipeState(tokens[INDEX_SEVEN], WEB_LIST_FLING_TAG) == SWIPE_START_STATE) {
                    LOG_D("Web list fling start");
                    startTime = GetTime(tokens[INDEX_FIVE]);
                } else if (GetSwipeState(tokens[INDEX_SEVEN], WEB_LIST_FLING_TAG) == SWIPE_FINISH_STATE) {
                    LOG_D("Web list fling finish");
                    endTime = GetTime(tokens[INDEX_FIVE]);
                }
            } else if (line.find(APP_SWIPER_SCROLL_TAG) != string::npos &&
                        GetSwipeState(tokens[INDEX_SEVEN], APP_SWIPER_SCROLL_TAG) == SWIPE_START_STATE) {
                LOG_D("Swiper scroll start");
                startTime = GetTime(tokens[INDEX_FIVE]);
            } else if (line.find(APP_SWIPER_FLING_TAG) != string::npos && startTime > INITIAL_VALUE &&
                        GetSwipeState(tokens[INDEX_SEVEN], APP_SWIPER_FLING_TAG) == SWIPE_FINISH_STATE) {
                LOG_D("Swiper scroll finish");
                endTime = GetTime(tokens[INDEX_FIVE]);
            } else if (line.find(DO_COMPOSITION_TAG) != string::npos && startTime > INITIAL_VALUE) {
                frameCount++;
            }
            if (startTime > INITIAL_VALUE && endTime > INITIAL_VALUE && frameCount != ZERO) {
                file.close();
                return frameCount / (endTime - startTime);
            }
        }
        file.close();
        return INVALID_VALUE;
    }

    string ListSwipeFpsCollection::GetSwipeState(const string token, const string traceTag)
    {
        vector<string> tagInfos = SplitString(token, '|');
        if (tagInfos[INDEX_TWO] == traceTag && tagInfos[INDEX_ONE] == to_string(pid_)) {
            switch (tagInfos[INDEX_ZERO][INDEX_ZERO]) {
                case 'S':
                    return SWIPE_START_STATE;
                case 'F':
                    return SWIPE_FINISH_STATE;
                default:
                    return SWIPE_INVALID_STATE;
            }
        }
        return SWIPE_INVALID_STATE;
    }

    double ListSwipeFpsCollection::GetTime(const string token)
    {
        string timeStr = token.substr(0, token.size() - 1);
        double time = INITIAL_VALUE;
        istringstream iss(timeStr);
        if ((iss >> time) && iss.eof()) {
            return time;
        }
        return time;
    }

    vector<string> ListSwipeFpsCollection::SplitString(const string line, const char delimiter)
    {
        stringstream ss(line);
        vector<std::string> tokens;
        string token;
        while (getline(ss, token, delimiter)) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }
}
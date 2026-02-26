/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef COMMON_UTILITIES
#define COMMON_UTILITIES

#include <array>
#include <string>
#include <string_view>
#include <cstdint>
#include <iostream>

#ifdef __OHOS__
#include "hilog/log.h"
#endif

// Undefine EXIT_SUCCESS and EXIT_FAILURE from stdlib.h to use our own definitions
#undef EXIT_SUCCESS
#undef EXIT_FAILURE

namespace OHOS::testhelper {
    // log tag length limit
    constexpr uint8_t MAX_LOG_TAG_LEN = 64;
    // testhelper exit codes
    constexpr int32_t EXIT_SUCCESS = 0;
    constexpr int32_t EXIT_FAILURE = 1;
    constexpr int32_t EXIT_SERVICE_UNAVAILABLE = 2;
    constexpr int32_t EXIT_NOT_SUPPORTED = 3;
    // constants
    constexpr int32_t TWO = 2;
    constexpr int32_t MS_PER_SECOND = 1000;
    constexpr int32_t TIME_BUFFER_SIZE = 80;
    constexpr int32_t BASE_YEAR = 1900;
    constexpr int32_t BASE_MONTH = 1;
    constexpr int32_t MIN_YEAR = 1970;
    constexpr int32_t MIN_MONTH = 1;
    constexpr int32_t MAX_MONTH = 12;
    constexpr int32_t MIN_DAY = 1;
    constexpr int32_t MAX_DAY = 31;
    constexpr int32_t MIN_HOUR = 0;
    constexpr int32_t MAX_HOUR = 23;
    constexpr int32_t MIN_MINUTE = 0;
    constexpr int32_t MAX_MINUTE = 59;
    constexpr int32_t MIN_SECOND = 0;
    constexpr int32_t MAX_SECOND = 59;
    constexpr int64_t MAX_TIMESTAMP_SECONDS = 2147483647;

    // 打印到控制台
    void PrintToConsole(const std::string& message);

    /**Generates log-tag by fileName and functionName*/
    constexpr std::array<char, MAX_LOG_TAG_LEN> GenLogTag(std::string_view fp, std::string_view func)
    {
        constexpr uint8_t MAX_CONTENT_LEN = MAX_LOG_TAG_LEN - 1;
        std::array<char, MAX_LOG_TAG_LEN> chars = {0};
        size_t pos = fp.find_last_of('/');
        if (pos == std::string_view::npos) {
            pos = 0;
        }
        uint8_t writeCursor = 0;
        chars[writeCursor++] = '[';
        for (size_t offSet = pos + 1; offSet < fp.length() && writeCursor < MAX_CONTENT_LEN; offSet++) {
            chars[writeCursor++] = fp[offSet];
        }
        if (writeCursor < MAX_CONTENT_LEN) {
            chars[writeCursor++] = ':';
        }
        if (writeCursor < MAX_CONTENT_LEN) {
            chars[writeCursor++] = '(';
        }
        for (size_t offSet = 0; offSet < func.length() && writeCursor < MAX_CONTENT_LEN; offSet++) {
            chars[writeCursor++] = func[offSet];
        }
        if (writeCursor < MAX_CONTENT_LEN) {
            chars[writeCursor++] = ')';
        }
        if (writeCursor < MAX_CONTENT_LEN) {
            chars[writeCursor++] = ']';
        }
        // record the actual tag-length in the end byte
        chars[MAX_CONTENT_LEN] = writeCursor;
        return chars;
    }

    // log level
    enum LogRank : uint8_t {
        DEBUG = 3, INFO = 4, WARN = 5, ERROR = 6
    };

#ifdef __OHOS__
#ifndef LOG_TAG
#define LOG_TAG "TestHelper"
#endif
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD003130
// print pretty log with pretty format, auto-generate tag by fileName and functionName at compile time
#define LOG(LEVEL, FMT, VARS...) do { \
    static constexpr auto tagChars= GenLogTag(__FILE__, __FUNCTION__); \
    static constexpr int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];   \
    if constexpr (tagLen > 0) { \
        auto tag = std::string_view(tagChars.data(), tagLen); \
        static constexpr LogType type = LogType::LOG_CORE; \
        HILOG_##LEVEL(type, "%{public}s " FMT, tag.data(), ##VARS); \
    } \
}while (0)
#else
// nop logger
#define LOG(LEVEL, FMT, VARS...) do {}while (0)
#endif

// print debug log
#define LOG_D(FMT, VARS...) LOG(DEBUG, FMT, ##VARS)
// print info log
#define LOG_I(FMT, VARS...) LOG(INFO, FMT, ##VARS)
// print warning log
#define LOG_W(FMT, VARS...) LOG(WARN, FMT, ##VARS)
// print error log
#define LOG_E(FMT, VARS...) LOG(ERROR, FMT, ##VARS)
}

#endif

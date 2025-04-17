/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef COMMON_UTILITIES_HPP
#define COMMON_UTILITIES_HPP

#include <array>
#include <chrono>
#include <string_view>
#include "nlohmann/json.hpp"

#ifdef __OHOS__
#include "hilog/log.h"
#endif

#define FORCE_INLINE __attribute__((always_inline)) inline

namespace OHOS::uitest {
    using CStr = const char *;
    constexpr size_t INDEX_ZERO = 0;
    constexpr size_t INDEX_ONE = 1;
    constexpr size_t INDEX_TWO = 2;
    constexpr size_t INDEX_THREE = 3;
    constexpr size_t INDEX_FOUR = 4;
    constexpr size_t INDEX_FIVE = 5;
    constexpr size_t INDEX_SIX = 6;
    constexpr size_t INDEX_SEVEN = 7;
    constexpr size_t INDEX_EIGHT = 8;
    constexpr size_t INDEX_NINE = 9;
    constexpr int32_t ZERO = 0;
    constexpr int32_t ONE = 1;
    constexpr int32_t TWO = 2;
    constexpr int32_t THREE = 3;
    constexpr int32_t FOUR = 4;
    constexpr int32_t FIVE = 5;
    constexpr int32_t SIX = 6;
    constexpr int32_t SEVEN = 7;
    constexpr int32_t EIGHT = 8;
    constexpr int32_t UNASSIGNED = -1;
    constexpr int32_t VIRTUAL_DISPLAY_ID = 999;

    /**Get current time millisecond.*/
    inline uint64_t GetCurrentMillisecond()
    {
        using namespace std::chrono;
        return time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();
    }

    /**Get current time microseconds.*/
    inline uint64_t GetCurrentMicroseconds()
    {
        using namespace std::chrono;
        return time_point_cast<microseconds>(steady_clock::now()).time_since_epoch().count();
    }

    inline void ReadInputModeFromJson(const nlohmann::json &json, bool &paste, bool &additional)
    {
        if (!json.empty()) {
            if (json.contains("paste") && json["paste"].is_boolean()) {
                paste = json["paste"];
            }
            if (json.contains("additional") && json["additional"].is_boolean()) {
                additional = json["additional"];
            }
        }
    }

    template <typename T> T ReadArgFromJson(const nlohmann::json &json, const std::string arg, const T defValue)
    {
        if (json.type() == nlohmann::detail::value_t::object && json.contains(arg)) {
            nlohmann::json val = json[arg];
            return val.get<T>();
        }
        return defValue;
    }

    // log tag length limit
    constexpr uint8_t MAX_LOG_TAG_LEN = 64;

    /**Generates log-tag by fileName and lineNumber, must be 'constexpr' to ensure the efficiency of Logger.*/
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
#define LOG_TAG "UiTestKit"
#endif
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD003100
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

#define DCHECK(cond) \
do { \
    if (!(cond)) { \
        LOG_E("DCHECK FAILED, %{public}s %{public}d: %{public}s", __FILE__, __LINE__, #cond); \
        _Exit(0); \
    } \
} while (0)

#endif
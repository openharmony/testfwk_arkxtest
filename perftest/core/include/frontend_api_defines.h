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

#ifndef FRONTEND_API_DEFINES_H
#define FRONTEND_API_DEFINES_H

#include <initializer_list>
#include <string_view>
#include <map>
#include "nlohmann/json.hpp"

namespace OHOS::perftest {
    using namespace std;
    enum ConnectionStat : uint8_t { UNINIT, CONNECTED, DISCONNECTED };
    constexpr string_view PUBLISH_EVENT_PREFIX = "perftest.api.caller.publish#";
    constexpr uint32_t WAIT_CONN_TIMEOUT_MS = 5000;
    constexpr uint32_t PUBLISH_MAX_RETIES = 10;
    constexpr uint32_t TEST_ITERATIONS = 5;
    constexpr uint32_t EXECUTION_TIMEOUT = 10000;

    enum ErrCode : uint32_t {
        /**No Error*/
        NO_ERROR = 0,
        /**Initialize failed.*/
        ERR_INITIALIZE_FAILED = 32400001,
        /**System error.*/
        ERR_INTERNAL = 32400002,
        /**Invalid input parameter.*/
        ERR_INVALID_INPUT = 32400003,
        /**Execute callback failed.*/
        ERR_CALLBACK_FAILED = 32400004,
        /**Collect metric data failed.*/
        ERR_DATA_COLLECTION_FAILED = 32400005,
        /**Get measure result failed.*/
        ERR_GET_RESULT_FAILED = 32400006,
        /**API does not allow calling concurrently.*/
        ERR_API_USAGE = 32400007,
    };

    const std::map<ErrCode, std::string> ErrDescMap = {
        /**Correspondence between error codes and descriptions*/
        {NO_ERROR, "No Error"},
        {ERR_INITIALIZE_FAILED, "Initialization failed."},
        {ERR_INTERNAL, "Internal error."},
        {ERR_INVALID_INPUT, "Invalid input parameter."},
    };

    /**API invocation error detail wrapper.*/
    struct ApiCallErr {
    public:
        ErrCode code_;
        std::string message_ = "";

        ApiCallErr() = delete;

        explicit ApiCallErr(ErrCode ec)
        {
            code_ = ec;
            message_ = ErrDescMap.find(ec)->second;
        }

        ApiCallErr(ErrCode ec, std::string_view msg)
        {
            code_ = ec;
            message_ = std::string(msg);
        }
    };

    /**Structure wraps the api-call data.*/
    struct ApiCallInfo {
        std::string apiId_;
        std::string callerObjRef_;
        nlohmann::json paramList_ = nlohmann::json::array();
    };

    /**Structure wraps the api-call reply.*/
    struct ApiReplyInfo {
        nlohmann::json resultValue_ = nullptr;
        ApiCallErr exception_ = ApiCallErr(NO_ERROR);
    };

    /** Specifications of a frontend enumerator value.*/
    struct FrontendEnumValueDef {
        std::string_view name_;
        std::string_view valueJson_;
    };

    /** Specifications of a frontend enumerator.*/
    struct FrontendEnumeratorDef {
        std::string_view name_;
        const FrontendEnumValueDef *values_;
        size_t valueCount_;
    };

    /** Specifications of a frontend json data property.*/
    struct FrontEndJsonPropDef {
        std::string_view name_;
        std::string_view type_;
        bool required_;
    };
    /** Specifications of a frontend json object.*/
    struct FrontEndJsonDef {
        std::string_view name_;
        const FrontEndJsonPropDef *props_;
        size_t propCount_;
    };

    /** Specifications of a frontend class method.*/
    struct FrontendMethodDef {
        std::string_view name_;
        std::string_view signature_;
        bool static_;
        bool fast_;
    };

    /** Specifications of a frontend class.*/
    struct FrontEndClassDef {
        std::string_view name_;
        const FrontendMethodDef *methods_;
        size_t methodCount_;
        bool bindClassObject_;
    };

    /** PerfMetric enumerator definition.*/
    constexpr FrontendEnumValueDef PERF_METRIC_VALUES[] = {
        {"DURATION", "0"},
        {"CPU_LOAD", "1"},
        {"CPU_USAGE", "2"},
        {"MEMORY_RSS", "3"},
        {"MEMORY_PSS", "4"},
        {"APP_START_RESPONSE_TIME", "5"},
        {"APP_START_COMPLETE_TIME", "6"},
        {"PAGE_SWITCH_COMPLETE_TIME", "7"},
        {"LIST_SWIPE_FPS", "8"},
    };
    constexpr FrontendEnumeratorDef PERF_METRIC_DEF = {
        "PerfMetric",
        PERF_METRIC_VALUES,
        sizeof(PERF_METRIC_VALUES) / sizeof(FrontendEnumValueDef),
    };

    /** PerfTestStrategy jsonObject definition.*/
    constexpr FrontEndJsonPropDef PERF_TEST_STRATEGY_PROPERTIES[] = {
        // Callback<<Callback<boolean>> saved in js, works as callbackRef in c++.
        {"metrics", "[int]", true},
        {"actionCode", "string", true},
        {"resetCode", "string", false},
        {"bundleName", "string", false},
        {"iterations", "int", false},
        {"timeout", "int", false},
    };
    constexpr FrontEndJsonDef PERF_TEST_STRATEGY_DEF = {
        "PerfTestStrategy",
        PERF_TEST_STRATEGY_PROPERTIES,
        sizeof(PERF_TEST_STRATEGY_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** PerfMeasureResult jsonObject definition.*/
    constexpr FrontEndJsonPropDef PERF_MEASURE_RESULT_PROPERTIES[] = {
        {"metric", "int", true},
        {"roundValues", "[int]", true},
        {"maximum", "int", true},
        {"minimum", "int", true},
        {"average", "int", true},
    };
    constexpr FrontEndJsonDef PERF_MEASURE_RESULT_DEF = {
        "PerfMeasureResult",
        PERF_MEASURE_RESULT_PROPERTIES,
        sizeof(PERF_MEASURE_RESULT_PROPERTIES) / sizeof(FrontEndJsonPropDef),
    };

    /** PerfTest class definition.*/
    constexpr FrontendMethodDef PERF_TEST_METHODS[] = {
        {"PerfTest.create", "(PerfTestStrategy):PerfTest", true, true},
        {"PerfTest.run", "():void", false, false},
        {"PerfTest.getMeasureResult", "(int):PerfMeasureResult", false, true},
        {"PerfTest.destroy", "():void", false, true},
    };
    constexpr FrontEndClassDef PERF_TEST_DEF = {
        "PerfTest",
        PERF_TEST_METHODS,
        sizeof(PERF_TEST_METHODS) / sizeof(FrontendMethodDef),
    };

    /** List all the frontend data-type definitions.*/
    const auto FRONTEND_CLASS_DEFS = {&PERF_TEST_DEF};
    const auto FRONTEND_ENUMERATOR_DEFS = {&PERF_METRIC_DEF};
    const auto FRONTEND_JSON_DEFS = {&PERF_TEST_STRATEGY_DEF, &PERF_MEASURE_RESULT_DEF};
    /** The allowed in/out data type scope of frontend apis.*/
    const std::initializer_list<std::string_view> DATA_TYPE_SCOPE = {
        "int",
        "float",
        "bool",
        "string",
        PERF_TEST_DEF.name_,
        PERF_METRIC_DEF.name_,
        PERF_TEST_STRATEGY_DEF.name_,
    };
} // namespace OHOS::perftest
#endif
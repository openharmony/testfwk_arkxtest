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

#include <sstream>
#include <unistd.h>
#include <regex.h>
#include <cstdio>
#include <cstdlib>
#include "frontend_api_handler.h"
#include "perf_test.h"

namespace OHOS::perftest {
    using namespace std;
    using namespace nlohmann;
    using namespace nlohmann::detail;

    static mutex g_gcQueueMutex;

    class PerfTestCallbackFowarder : public PerfTestCallback {
    public:
        PerfTestCallbackFowarder() {};
        void OnCall(const string&& codeRef, const int32_t timeout, ApiCallErr &error)
        {
            LOG_I("%{public}s called, codeRef = %{public}s, timeout = %{public}d", __func__, codeRef.c_str(), timeout);
            if (codeRef == "") {
                LOG_W("Callback have not been defined");
                return;
            }
            const auto &server = FrontendApiServer::Get();
            ApiCallInfo in;
            ApiReplyInfo out;
            in.apiId_ = "PerfTest.run";
            in.paramList_.push_back(codeRef);
            in.paramList_.push_back(timeout);
            server.Callback(in, out);
            error = out.exception_;
        }
        void OnDestory(const list<string> codeRefs, ApiCallErr &error)
        {
            const auto &server = FrontendApiServer::Get();
            ApiCallInfo in;
            ApiReplyInfo out;
            in.apiId_ = "PerfTest.destory";
            in.paramList_.push_back(codeRefs);
            server.Callback(in, out);
        }
    };

    /** API argument type list map.*/
    static multimap<string, pair<vector<string>, size_t>> sApiArgTypesMap;

    static void ParseMethodSignature(string_view signature, vector<string> &types, size_t &defArg)
    {
        int charIndex = 0;
        constexpr size_t benLen = 32;
        char buf[benLen];
        size_t tokenLen = 0;
        size_t defArgCount = 0;
        string token;
        for (char ch : signature) {
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '[') {
                buf[tokenLen++] = ch;
            } else if (ch == '?') {
                defArgCount++;
            } else if (ch == ',' || ch == '?' || ch == ')') {
                if (tokenLen > 0) {
                    token = string_view(buf, tokenLen);
                    DCHECK(find(DATA_TYPE_SCOPE.begin(), DATA_TYPE_SCOPE.end(), token) != DATA_TYPE_SCOPE.end());
                    types.emplace_back(token);
                }
                tokenLen = 0; // consume token and reset buffer
                if (ch == ')') {
                    // add return value type to the end of types.
                    string retType = string(signature.substr(charIndex + 2));
                    types.emplace_back(retType);
                    break; // end parsing
                }
            }
            charIndex++;
        }
        defArg = defArgCount;
    }

    /** Parse frontend method definitions to collect type information.*/
    static void ParseFrontendMethodsSignature()
    {
        for (auto classDef : FRONTEND_CLASS_DEFS) {
            LOG_I("parse class %{public}s", string(classDef->name_).c_str());
            if (classDef->methods_ == nullptr || classDef->methodCount_ <= 0) {
                continue;
            }
            for (size_t idx = 0; idx < classDef->methodCount_; idx++) {
                auto methodDef = classDef->methods_[idx];
                auto paramTypes = vector<string>();
                size_t hasDefaultArg = 0;
                ParseMethodSignature(methodDef.signature_, paramTypes, hasDefaultArg);
                sApiArgTypesMap.insert(make_pair(string(methodDef.name_), make_pair(paramTypes, hasDefaultArg)));
            }
        }
    }
 
    FrontendApiServer &FrontendApiServer::Get()
    {
        static FrontendApiServer server;
        return server;
    }

    void FrontendApiServer::AddHandler(string_view apiId, ApiInvokeHandler handler)
    {
        if (handler == nullptr) {
            return;
        }
        handlers_.insert(make_pair(apiId, handler));
    }

    void FrontendApiServer::SetCallbackHandler(ApiInvokeHandler handler)
    {
        callbackHandler_ = handler;
    }

    void FrontendApiServer::Callback(const ApiCallInfo& in, ApiReplyInfo& out) const
    {
        if (callbackHandler_ == nullptr) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "No callback handler set!");
            return;
        }
        LOG_I("%{public}s called", __func__);
        callbackHandler_(in, out);
    }

    bool FrontendApiServer::HasHandlerFor(std::string_view apiId) const
    {
        return handlers_.find(string(apiId)) != handlers_.end();
    }

    void FrontendApiServer::RemoveHandler(string_view apiId)
    {
        handlers_.erase(string(apiId));
    }

    void FrontendApiServer::AddCommonPreprocessor(string_view name, ApiInvokeHandler processor)
    {
        if (processor == nullptr) {
            return;
        }
        commonPreprocessors_.insert(make_pair(name, processor));
    }

    void FrontendApiServer::RemoveCommonPreprocessor(string_view name)
    {
        commonPreprocessors_.erase(string(name));
    }

    void FrontendApiServer::Call(const ApiCallInfo &in, ApiReplyInfo &out) const
    {
        LOG_I("Begin to invoke api '%{public}s', '%{public}s'", in.apiId_.data(), in.paramList_.dump().data());
        auto call = in;
        // initialize method signature
        if (sApiArgTypesMap.empty()) {
            ParseFrontendMethodsSignature();
        }
        auto find = handlers_.find(call.apiId_);
        if (find == handlers_.end()) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "No handler found for api '" + call.apiId_ + "'");
            return;
        }
        try {
            for (auto &[name, processor] : commonPreprocessors_) {
                processor(call, out);
                if (out.exception_.code_ != NO_ERROR) {
                    out.exception_.message_ = "(PreProcessing: " + name + ")" + out.exception_.message_;
                    return; // error during pre-processing, abort
                }
            }
        } catch (std::exception &ex) {
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Preprocessor failed: " + string(ex.what()));
        }
        try {
            find->second(call, out);
        } catch (std::exception &ex) {
            // catch possible json-parsing error
            out.exception_ = ApiCallErr(ERR_INTERNAL, "Handler failed: " + string(ex.what()));
        }
    }

    void ApiTransact(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        LOG_I("Begin to invoke api '%{public}s', '%{public}s'", in.apiId_.data(), in.paramList_.dump().data());
        FrontendApiServer::Get().Call(in, out);
    }

    /** Backend objects cache.*/
    static map<string, unique_ptr<BackendClass>> sBackendObjects;
    /** PerfTest binding map.*/
    static map<string, string> sPerfTestBindingMap;


#define CHECK_CALL_ARG(condition, code, message, error) \
    if (!(condition)) {                                 \
        error = ApiCallErr((code), (message));          \
        return true;                                         \
    }

    static bool CheckCallArgType(string_view expect, const json &value, bool isDefAgc, ApiCallErr &error);

    static bool CheckCallArgClassType(string_view expect, const json &value, ApiCallErr &error)
    {
        auto begin = FRONTEND_CLASS_DEFS.begin();
        auto end = FRONTEND_CLASS_DEFS.end();
        auto find = find_if(begin, end, [&expect](const FrontEndClassDef *def) { return def->name_ == expect; });
        if (find == end) {
            return false;
        }
        const auto type = value.type();
        CHECK_CALL_ARG(type == value_t::string, ERR_INVALID_INPUT, "Expect " + string(expect), error);
        const auto findRef = sBackendObjects.find(value.get<string>());
        CHECK_CALL_ARG(findRef != sBackendObjects.end(), ERR_INTERNAL, "Bad object ref", error);
        return true;
    }

    static bool CheckCallArgJsonType(string_view expect, const json &value, ApiCallErr &error)
    {
        auto begin = FRONTEND_JSON_DEFS.begin();
        auto end = FRONTEND_JSON_DEFS.end();
        auto find = find_if(begin, end, [&expect](const FrontEndJsonDef *def) { return def->name_ == expect; });
        if (find == end) {
            return false;
        }
        const auto type = value.type();
        CHECK_CALL_ARG(type == value_t::object, ERR_INVALID_INPUT, "Expect " + string(expect), error);
        auto copy = value;
        for (size_t idx = 0; idx < (*find)->propCount_; idx++) {
            auto def = (*find)->props_ + idx;
            const auto propName = string(def->name_);
            if (!value.contains(propName)) {
                CHECK_CALL_ARG(!(def->required_), ERR_INVALID_INPUT, "Missing property " + propName, error);
                continue;
            }
            copy.erase(propName);
            // check json property value type recursive
            CheckCallArgType(def->type_, value[propName], !def->required_, error);
            if (error.code_ != NO_ERROR) {
                error.message_ = "Illegal value of property '" + propName + "': " + error.message_;
                return true;
            }
        }
        CHECK_CALL_ARG(copy.empty(), ERR_INVALID_INPUT, "Illegal property of " + string(expect), error);
        return true;
    }

    static bool CheckCallArgArrayType(string_view expect, const json &value, ApiCallErr &error)
    {
        if (expect.front() != '[' || expect.back() != ']') {
            return false;
        }
        const auto type = value.type();
        const auto isArray = type == value_t::array;
        if (!isArray) {
            error = ApiCallErr(ERR_INVALID_INPUT, "Expect array");
            return true;
        }
        string_view expectParaType = expect.substr(1, expect.size() - 2);
        for (auto& para : value) {
            CheckCallArgType(expectParaType, para, false, error);
            if (error.code_ != NO_ERROR) {
                return true;
            }
        }
        return true;
    }

    /** Check if the json value represents and illegal data of expected type.*/
    static bool CheckCallArgType(string_view expect, const json &value, bool isDefAgc, ApiCallErr &error)
    {
        const auto type = value.type();
        if (isDefAgc && type == value_t::null) {
            return true;
        }
        if (CheckCallArgClassType(expect, value, error) || CheckCallArgJsonType(expect, value, error) ||
            CheckCallArgArrayType(expect, value, error)) {
            return true;
        }
        const auto isInteger = type == value_t::number_integer || type == value_t::number_unsigned;
        if (expect == "int") {
            CHECK_CALL_ARG(isInteger, ERR_INVALID_INPUT, "Expect integer", error);
            if (atoi(value.dump().c_str()) < 0) {
                error = ApiCallErr(ERR_INVALID_INPUT, "Expect integer which cannot be less than 0");
                return true;
            }
        } else if (expect == "float") {
            CHECK_CALL_ARG(isInteger || type == value_t::number_float, ERR_INVALID_INPUT, "Expect float", error);
        } else if (expect == "bool") {
            CHECK_CALL_ARG(type == value_t::boolean, ERR_INVALID_INPUT, "Expect boolean", error);
        } else if (expect == "string") {
            CHECK_CALL_ARG(type == value_t::string, ERR_INVALID_INPUT, "Expect string", error);
        } else {
            CHECK_CALL_ARG(false, ERR_INTERNAL, "Unknown target type " + string(expect), error);
        }
        return true;
    }

    /** Checks ApiCallInfo data, deliver exception and abort invocation if check fails.*/
    static void APiCallInfoChecker(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        auto count = sApiArgTypesMap.count(in.apiId_);
        // return nullptr by default
        out.resultValue_ = nullptr;
        auto find = sApiArgTypesMap.find(in.apiId_);
        size_t index = 0;
        while (index < count) {
            if (find == sApiArgTypesMap.end()) {
                return;
            }
            bool checkArgNum = false;
            bool checkArgType = true;
            out.exception_ = {NO_ERROR, "No Error"};
            auto &types = find->second.first;
            auto argSupportDefault = find->second.second;
            // check argument count.(last item of "types" is return value type)
            auto maxArgc = types.size() - 1;
            auto minArgc = maxArgc - argSupportDefault;
            auto argc = in.paramList_.size();
            checkArgNum = argc <= maxArgc && argc >= minArgc;
            if (!checkArgNum) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Illegal argument count");
                ++find;
                ++index;
                continue;
            }
            // check argument type
            for (size_t idx = 0; idx < argc; idx++) {
                auto isDefArg = (idx >= minArgc) ? true : false;
                CheckCallArgType(types.at(idx), in.paramList_.at(idx), isDefArg, out.exception_);
                if (out.exception_.code_ != NO_ERROR) {
                    out.exception_.message_ = "Check arg" + to_string(idx) + " failed: " + out.exception_.message_;
                    checkArgType = false;
                    break;
                }
            }
            if (checkArgType) {
                return;
            }
            ++find;
            ++index;
        }
    }

    /** Store the backend object and return the reference-id.*/
    static string StoreBackendObject(unique_ptr<BackendClass> ptr, string_view ownerRef = "")
    {
        static map<string, uint32_t> sObjectCounts;
        DCHECK(ptr != nullptr);
        const auto typeName = string(ptr->GetFrontendClassDef().name_);
        auto find = sObjectCounts.find(typeName);
        uint32_t index = 0;
        if (find != sObjectCounts.end()) {
            index = find->second;
        }
        auto ref = typeName + "#" + to_string(index);
        sObjectCounts[typeName] = index + 1;
        sBackendObjects[ref] = move(ptr);
        if (!ownerRef.empty()) {
            DCHECK(sBackendObjects.find(string(ownerRef)) != sBackendObjects.end());
            sPerfTestBindingMap[ref] = ownerRef;
        }
        return ref;
    }

    /** Retrieve the stored backend object by reference-id.*/
    template <typename T, typename = enable_if<is_base_of_v<BackendClass, T>>>
    static T *GetBackendObject(string_view ref, ApiCallErr &error)
    {
        auto find = sBackendObjects.find(string(ref));
        if (find == sBackendObjects.end() || find->second == nullptr) {
            error = ApiCallErr(ERR_INTERNAL, "Object does not exist");
            return nullptr;
        }
        return reinterpret_cast<T *>(find->second.get());
    }

    /** Delete stored backend objects.*/
    static void BackendObjectsCleaner(const ApiCallInfo &in, ApiReplyInfo &out)
    {
        stringstream ss("Deleted objects[");
        DCHECK(in.paramList_.type() == value_t::array);
        for (const auto &item : in.paramList_) {
            DCHECK(item.type() == value_t::string); // must be objRef
            const auto ref = item.get<string>();
            auto findBinding = sPerfTestBindingMap.find(ref);
            if (findBinding != sPerfTestBindingMap.end()) {
                sPerfTestBindingMap.erase(findBinding);
            }
            auto findObject = sBackendObjects.find(ref);
            if (findObject == sBackendObjects.end()) {
                LOG_W("No such object living: %{public}s", ref.c_str());
                continue;
            }
            sBackendObjects.erase(findObject);
            ss << ref << ",";
        }
        ss << "]";
        LOG_I("%{public}s", ss.str().c_str());
    }

    template <typename T> static T ReadCallArg(const ApiCallInfo &in, size_t index)
    {
        DCHECK(in.paramList_.type() == value_t::array);
        DCHECK(index <= in.paramList_.size());
        return in.paramList_.at(index).get<T>();
    }

    template <typename T> static T ReadCallArg(const ApiCallInfo &in, size_t index, T defValue)
    {
        DCHECK(in.paramList_.type() == value_t::array);
        if (index >= in.paramList_.size()) {
            return defValue;
        }
        auto type = in.paramList_.at(index).type();
        if (type == value_t::null) {
            return defValue;
        } else {
            return in.paramList_.at(index).get<T>();
        }
    }

    static void RegisterPerfTestCreate()
    {
        auto &server = FrontendApiServer::Get();
        auto create = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto perfTestStrategyJson = ReadCallArg<json>(in, INDEX_ZERO);
            if (perfTestStrategyJson.empty()) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "PerfTestStrategy cannot be empty");
                return;
            }
            auto metricsArray = perfTestStrategyJson["metrics"];
            if (!metricsArray.is_array() || metricsArray.empty()) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Metrics cannot be empty");
                return;
            }
            set<PerfMetric> metrics;
            for (auto& metricNum : metricsArray) {
                auto metric = metricNum.get<PerfMetric>();
                if (metric < ZERO || metric >= PerfMetric::METRIC_COUNT) {
                    out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Illegal perfMetric");
                    return;
                }
                metrics.insert(metric);
            }
            auto actionCodeRef = perfTestStrategyJson["actionCode"];
            auto resetCodeRef = ReadArgFromJson<string>(perfTestStrategyJson, "resetCode", "");
            auto bundleName = ReadArgFromJson<string>(perfTestStrategyJson, "bundleName", "");
            auto iterations = ReadArgFromJson<int32_t>(perfTestStrategyJson, "iterations", TEST_ITERATIONS);
            auto timeout = ReadArgFromJson<int32_t>(perfTestStrategyJson, "timeout", EXECUTION_TIMEOUT);
            auto perfTestStrategy = make_unique<PerfTestStrategy>(metrics, actionCodeRef, resetCodeRef, bundleName,
                                                                    iterations, timeout);
            auto perfTestCallback = make_unique<PerfTestCallbackFowarder>();
            auto perfTest = make_unique<PerfTest>(move(perfTestStrategy), move(perfTestCallback));
            out.resultValue_ = StoreBackendObject(move(perfTest));
        };
        server.AddHandler("PerfTest.create", create);
    }

    static void RegisterPerfTestRun()
    {
        auto &server = FrontendApiServer::Get();
        auto run = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto perfTest = GetBackendObject<PerfTest>(in.callerObjRef_, out.exception_);
            if (out.exception_.code_ != NO_ERROR) {
                return;
            }
            perfTest->RunTest(out.exception_);
        };
        server.AddHandler("PerfTest.run", run);
    }

    static void RegisterGetMeasureResult()
    {
        auto &server = FrontendApiServer::Get();
        auto getMeasureResult = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto perfTest = GetBackendObject<PerfTest>(in.callerObjRef_, out.exception_);
            if (out.exception_.code_ != NO_ERROR) {
                return;
            }
            auto metric = ReadCallArg<PerfMetric>(in, INDEX_ZERO);
            if (metric < ZERO || metric >= PerfMetric::METRIC_COUNT) {
                out.exception_ = ApiCallErr(ERR_INVALID_INPUT, "Illegal perfMetric");
                return;
            }
            if (perfTest->IsMeasureRunning()) {
                out.exception_ = ApiCallErr(ERR_INTERNAL, "Measure is running, can not get measure result now");
                return;
            }
            json resData = perfTest->GetMeasureResult(metric, out.exception_);
            out.resultValue_ = resData;
        };
        server.AddHandler("PerfTest.getMeasureResult", getMeasureResult);
    }

    static void RegisterPerfTestDestory()
    {
        auto &server = FrontendApiServer::Get();
        auto destory = [](const ApiCallInfo &in, ApiReplyInfo &out) {
            auto perfTest = GetBackendObject<PerfTest>(in.callerObjRef_, out.exception_);
            if (out.exception_.code_ != NO_ERROR) {
                return;
            }
            if (perfTest->IsMeasureRunning()) {
                out.exception_ = ApiCallErr(ERR_INTERNAL, "Measure is running, can not destory now");
                return;
            }
            perfTest->Destory(out.exception_);
            auto gcCall = ApiCallInfo {.apiId_ = "BackendObjectsCleaner"};
            unique_lock<mutex> lock(g_gcQueueMutex);
            gcCall.paramList_.emplace_back(in.callerObjRef_);
            lock.unlock();
            auto gcReply = ApiReplyInfo();
            BackendObjectsCleaner(gcCall, gcReply);
        };
        server.AddHandler("PerfTest.destory", destory);
    }

    /** Register frontendApiHandlers and preprocessors on startup.*/
    __attribute__((constructor)) static void RegisterApiHandlers()
    {
        auto &server = FrontendApiServer::Get();
        server.AddCommonPreprocessor("APiCallInfoChecker", APiCallInfoChecker);
        server.AddHandler("BackendObjectsCleaner", BackendObjectsCleaner);
        RegisterPerfTestCreate();
        RegisterPerfTestRun();
        RegisterGetMeasureResult();
        RegisterPerfTestDestory();
    }
} // namespace OHOS::perftest

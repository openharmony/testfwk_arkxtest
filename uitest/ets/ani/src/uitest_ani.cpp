#include "ani.h"
#include <array>
#include <iostream>
#include <unistd.h>
#include <future>
#include <queue>
#include <set>
#include <string>
#include "json.hpp"
#include "fcntl.h"
#include "common_utilities_hpp.h"
#include "frontend_api_handler.h"
#include "ipc_transactor.h"
#include "test_server_client.h"
#include <cstring>

using namespace OHOS::uitest;
using namespace nlohmann;
using namespace std;
static ApiTransactor g_apiTransactClient(false);
static future<void> g_establishConnectionFuture;

template<typename T>
void compareAndReport(const T& param1, const T& param2, const std::string& errorMessage, const std::string& message) {
    if (param1 != param2) {
        std::cerr << errorMessage << std::endl;
        return;
    } else {
        std::cerr << message << std::endl;
        return;
    }
}

static string aniStringToStdString([[maybe_unused]] ani_env *env, ani_string string_object) {
    ani_size strSize;
    env->String_GetUTF8Size(string_object, &strSize);
    std::vector<char> buffer(strSize + 1);
    char* utf8_buffer = buffer.data();
    ani_size bytes_written = 0;
    env->String_GetUTF8(string_object, utf8_buffer, strSize + 1, &bytes_written);
    utf8_buffer[bytes_written]='\0';
    std::string s = std::string(utf8_buffer);
    std::cout<<s<<std::endl;
    return s;
}

static ani_class findCls(ani_env *env, const char *className) {
    ani_class cls;
    ani_ref nullref;
    env->GetNull(&nullref);
    if (ANI_OK != env->FindClass(className, &cls)) {
        std::cerr << "Not found className:" << className << std::endl;
    }
    return cls;
}

static ani_method findCtorMethod(ani_env *env, ani_class cls, const char *name) {
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", name, &ctor)) {
        std::cerr << "Not found ctor of:" << name << std::endl;
    }
    return ctor;
}

static ani_object CreateException(ani_env *env, uint32_t code, std::string msg) {
    ani_object errObj = nullptr;
    static const char* className = "Lescompat/Error;";
    ani_class errCls = findCls(env, className);
    ani_method errCtor = findCtorMethod(env, errCls, "Lstd/core/String;Lescompat/ErrorOptions;:V");
    ani_string result_string{};
    env->String_NewUTF8(msg.c_str(), msg.size(), &result_string);
    if (ANI_OK != env->Object_New(errCls, errCtor, &errObj, result_string)) {
        std::cerr<<"Create Object Failed" << className << std::endl;
        return errObj;
    }
    return errObj;
}

static ani_ref UnmarshalObject(ani_env *env, nlohmann::json resultValue_) {
    const auto resultType - resultValue_.type();
    ani_ref result = nullptr;
    if (resultType == nlohmann::detail::value_t::null) {
        return result;
    } else if (resultType!=nlohmann::detail::value_t::string) {
        ani_string str;
        env->String_NewUTF8(resultValue_.dump().c_str(), resultValue_.dump().size(), &str);
        result = reinterpret_cast<ani_ref>(str);
        return result;
    }
    const auto cppString = resultValue_.get<string>();
    string frontendTypeName;
    ani_string str;
    env->String_NewUTF8(cppString.c_str(), cppString.length(), &str);
    result = reinterpret_cast<ani_ref>(str);
    ani_size size;
    env->String_GetUTF16Size(str, &size);
    return result;    
}

static ani_ref UnmarshalReply(ani_env *env, const ApiCallInfo callInfo_, const ApiReplyInfo &reply_) {
    if (callInfo_.fdParamIndex_ >= 0) {
        auto fd = callInfo_.paramList_.at(INDEX_ZERO).get<int>();
        (void) close(fd);
    }
    LOG_D("Start to UnmarshalReply");
    const auto &message = reply_.exception_.message_;
    ErrCode code = reply_.exception_.code_;
    if (code == INTERNAL_ERROR || code == ERR_INTERNAL) {
        LOG_E("UItest : ERRORINFO: code='%{public}u', message='%{public}s'", code, message.c_str());
    } else if (reply_.exception_.code_ != NO_ERROR) {
        LOG_E("UItest : ERRORINFO: code='%{public}u', message='%{public}s'", code, message.c_str());
        ani_object err = CreateException(env, code, message);
        if (err) {
            env->ThrowError(err);
        }
        return err;
    }
    LOG_D("UITEST: Start to unmarshall return value:%{public}s", reply_.resultValue_.dump().c_str());
    const auto resultType = reply_.resultValue_.type();
    ani_ref result = nullptr;
    if (resultType == nlohmann::detail::value_t::null) {
        return result;
    } else if (resultType == nlohmann::detail::value_t::array) {
        ani_class arrayCls = nullptr;
        if (ANI_OK!=env->FindClass("Lescompat/Array;", &arrayCls)) {
            std::cerr<<"FindClass Array Failed" <<std::endl;
        }
        ani_ref undefinedRef = nullptr;
        if (ANI_OK != env->GetUndefined(&undefinedRef)) {
            std::cer << "GetUndefined Failed" << std::endl;
        }
        ani_method arrayCtor = findCtorMethod(env, arrayCls, "I:V");
        ani_object arrayObj;
        ani_size com_size = reply_.resultValue_.size();
        if (ANI_OK != env->Object_New(arrayCls, arrayCtor, &arrayObj, com_size)) {
            std::cerr<< "Object New Array Failed" <<std::endl;
            return reinterpret_cast<ani_ref>(arrayObj);
        }
        for (ani_size index = 0; index < reply_.resultValue_.size(); index++) {
            ani_ref item = UnmarshalObject(env, reply_.resultValue_.at(index));
            if (ANI_OK != env->Object_CallMethodByName_Void(arrayObj, "$_set", "ILstd/core/Object;:V", index, item)) {
                std::cerr<< "Object_CallMethodByName_Void set Failed" <<std::endl;
                break;
            }
        }
        return reinterpret_cast<ani_ref>(arrayObj);
    } else {
        return UnmarshalObject(env, reply_.resultValue_);
    }
}

static void waitForConnectionIfNeed() {
    if (g_establishConnectionFuture.valid()) {
        LOG_I(" Begin Wait for Connection");
        g_establishConnectionFuture.get();
    }
}

static void ScheduleEstablishConnection(ani_env *env, [[maybe_unused]]ani_class clazz, ani_string connToken) {
    auto token = aniStringToStdString(env, connToken);
    g_establishConnectionFuture = async(launch::async, [env, token]() {
        using namespace std::placeholders;
        auto result = g_apiTransactClient.InitAndConnectPeer(token, nullptr);
        LOG_I("End setup transaction connection, result=%{public}d", result);
        std::cout<<"End setup trasaction connection, result" << result << std::endl;
    });
    if (!OHOS::testserver::TestServerClient::GetInstance().StartUiTestDaemon()) {
        LOG_E("Start uitest-daemon failed");
    }
    waitForConnectionIfNeed();
    return;
}

static ani_string unwrapp(ani_env *env, ani_object object, const char *name) {
    ani_ref it;
    if (ANI_OK!= env->Object_GetFieldByName_Ref(object, name, &it)) {
        return nullptr;
    }
    return reinterpret_cast<ani_string>(it);
}

static json getPoint(ani_env *env, ani_object p) {
    auto point = json();
    static const char *className = "Luitest_ani/PointInner;";
    ani_class cls = findCls(env, className);
    ani_method xGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>x", nullptr, &xGetter)) {
        std::cerr << "Find Method <get>x failed" <<std::endl;
    }
    ani_int x;
    if (ANI_OK != env->Object_CallMethodByName_Int(p, xGetter, &x)) {
        std::cerr << "call xgetter failed" << className << std::endl;
        return point;
    }
    point["x"] = x;
    ani_method yGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>y", nullptr, &yGetter)) {
        std::cerr << "Find Method <get>y failed" <<std::endl;
    }
    ani_int y;
    if (ANI_OK != env->Object_CallMethodByName_Int(p, yGetter, &y)) {
        std::cerr << "call ygetter failed" << className << std::endl;
        return point;
    }
    point["y"] = y;
    return point;
}

static json getRect(ani_env *env, ani_object p) {
    auto rect = json();
    static const char *className = "Luitest_ani/PointInner;";
    ani_class cls = findCls(env, className);
    ani_method leftGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>left", nullptr, &leftGetter)) {
        std::cerr << "Find Method <get>left failed" <<std::endl;
    }
    ani_double left;
    if (ANI_OK != env->Object_CallMethodByName_Double(p, leftGetter, &left)) {
        std::cerr << "call leftgetter failed" << className << std::endl;
        return point;
    }
    rect["left"] = int(left);

    ani_method rightGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>right", nullptr, &rightGetter)) {
        std::cerr << "Find Method <get>right failed" <<std::endl;
    }
    ani_double right;
    if (ANI_OK != env->Object_CallMethodByName_Double(p, rightGetter, &right)) {
        std::cerr << "call rightgetter failed" << className << std::endl;
        return rect;
    }
    rect["right"] = int(right);

    ani_method topGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>top", nullptr, &topGetter)) {
        std::cerr << "Find Method <get>top failed" <<std::endl;
    }
    ani_double top;
    if (ANI_OK != env->Object_CallMethodByName_Double(p, topGetter, &top)) {
        std::cerr << "call topgetter failed" << className << std::endl;
        return rect;
    }
    rect["top"] = int(top);

        ani_method bottomGetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<get>bottom", nullptr, &bottomGetter)) {
        std::cerr << "Find Method <get>bottom failed" <<std::endl;
    }
    ani_double bottom;
    if (ANI_OK != env->Object_CallMethodByName_Double(p, bottomGetter, &bottom)) {
        std::cerr << "call bottomgetter failed" << className << std::endl;
        return rect;
    }
    rect["bottom"] = int(bottom);
    return rect;
}

static ani_object newRect(ani_env *env, ani_object object, nlohmann::json num) {
    ani_object rect_obj = {};
    static const char *className = "Luitest_ani/RectInner;";
    ani_class cls = findCls(env, className);
    ani_method ctor;
    if (cls) {
        static const char *name = nullptr;
        ctor = findCtorMethod(env, cls, name);
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &rect_obj)) {
        std::cerr<< "Create Rect object failed "<< std::endl;
        return rect_obj;
    }
    ani_method setter;
    string direct[] = {"left", "top", "right", "bottom" };
    for (int i=0; i<4; i++) {
        string tag = direct[i];
        if (ANI_OK != env->Class_FindMethod(cls, "<set>"+tag, nullptr, &setter)) {
            std::cerr << "Find Method <set>tag failed" <<std::endl;
        }
        if (ANI_OK != env->Object_CallMethod_Void(rect_obj, setter, ani_double(num[tag]))) {
            std::cerr << "call setter failed" << className << std::endl;
            return rect_obj;
        }
    }
    return rect_obj;
}

static ani_object newPoint(ani_env *env, ani_object obj, int x, int y) {
    ani_object point_obj = {};
    static const char *className = "Luitest_ani/PointInner;";
    ani_class cls = findCls(env, className);
    if (cls) {
        static const char *name = nullptr;
        ctor = findCtorMethod(env, cls, name);
    }    
    ani_method ctor;
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &point_obj)) {
        std::cerr<<"Create point object failed" << std::endl;
        return point_obj;
    }    
    ani_method setter;
    string direct[] = { "x", "y" };
    int num = {x, y};
    for (int i=0; i<2; i++) {
        string tag = direct[i];
        if (ANI_OK != env->Class_FindMethod(cls, "<set>"+tag, nullptr, &setter)) {
            std::cerr << "Find Method <set>tag failed" <<std::endl;
        }
        if (ANI_OK != env->Object_CallMethod_Void(point_obj, setter, ani_int(num[tag]))) {
            std::cerr << "call setter failed" << className << std::endl;
            return point_obj;
        }
    }
    return point_obj;
}

static ani_boolean BindInterfaces(ani_env *env) {
    static const char *className = "Luitest_ani/ETSGLOBAL;";
    ani_class cls = findCls(env, className);
    if (cls == nullptr) {
        return false;
    }
    std::array methods = {
        ani_native_function {"newPoint", nullptr, reinterpret_cast<void*>(newPoint)},
        ani_native_function {"newRect", nullptr, reinterpret_cast<void*>(newRect)},
        ani_native_function {"ScheduleEstablishConnection", nullptr, reinterpret_cast<void*>(ScheduleEstablishConnection)},
    }
    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        std::cerr << "Cannot bind native methods to " << className << sts::endl;
        return false;
    }
    return true;
}

static ani_ref createMatrix(ani_env *env, ani_object object) {
    static const char *className = "Luitest_ani/PointerMatrix;";
    ani_class cls = findCls(env, className);
    ani_ref nullref;
    env->GetNull(&nullref);
    ani_method ctor;
    if (cls) {
        ctor = findCtorMethod(env, cls, "Lstd/core/String;:V");
    } else {
        return nullref;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "PointerMatrix.create";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref nativePointerMatrix = UnmarshalReply(env, callInfo_, reply_);
    ani_object pointer_matrix_object;
    if (ANI_OK != env->Object_New(cls, ctor, &pointer_matrix_object, reinterpret_cast<ani_object>(nativePointerMatrix))) {
        std::cerr<<"New PointerMatrix Failed"<<std::endl;
    }
    return pointer_matrix_object;
}

static void setPoint(ani_env *env, ani_object object, ani_int finger, ani_int step, ani_object point) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_="PointerMatrix.setPoint";
    callInfo_.paramList_.push_back(finger);
    callInfo_.paramList_.push_back(step);
    callInfo_.paramList_.push_back(getPoint(env, point));
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, object, "nativePointerMatrix"));
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean BindPointMatrix(ani_env *env) {
    static const char *className = "Luitest_ani/PointerMatrix;";
    ani_class cls = findCls(env, className);
    if (cls == nullptr) {
        return false;
    }
    std::array methods = {
        ani_native_function {"create", nullptr, reinterpret_cast<void*>(createMatrix)},
        ani_native_function {"setPoint", nullptr, reinterpret_cast<void*>(setPoint)},
    }
    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        std::cerr << "Cannot bind native methods to " << className << sts::endl;
        return false;
    }
    return true;
}

static ani_ref createOn(ani_env *env, ani_object object, nlohmann::json params, string apiId_) {
    static const char *className = "Luitest_ani/On;";
    ani_class cls = findCls(env, className);
    ani_method ctor;
    if (cls) {
        ctor = findCtorMethod(env, cls, "Lstd/core/String;:V");
    }
    if (ctor == nullptr || cls== nullptr) {
        return nullptr;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = apiId_;
    callInfo_.paramList_ = params;
    string on_obj = aniStringToStdString(env, unwrapp(env, object, "nativeOn"));
    if (on_obj != "") {
        callInfo_.callerObjRef_ = on_obj;
    } else {
        callInfo_.callerObjRef_ = string(REF_SEED_BY);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref nativeOn = UnmarshalReply(env, callInfo_, reply_);
    ani_object on_obj;
    if (ANI_OK != env->Object_New(cls, ctor, reinterpret_cast<ani_ref>(nativeOn))) {
        std::cerr<< "New ON failed" <<std::endl;
    }
    return on_obj;
}

static ani_ref within(ani_env *env, ani_object obj, ani_object on) {
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.within");
}

static ani_ref isBefore(ani_env *env, ani_object obj, ani_object on) {
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.isBefore");
}

static ani_ref isAfter(ani_env *env, ani_object obj, ani_object on) {
    nlohmann::json params = nlohmann::json::array();
    string p = aniStringToStdString(env, unwrapp(env, on, "nativeOn"));
    params.push_back(p);
    return createOn(env, obj, params, "On.isAfter");
}

static ani_ref id(ani_env *env, ani_object obj, ani_string id, ani_int pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, id));
    params.push_back(pattern);
    return createOn(env, obj, params, "On.id");
}
static ani_ref text(ani_env *env, ani_object obj, ani_string text, ani_int pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    params.push_back(pattern);
    return createOn(env, obj, params, "On.text");
}

static ani_ref type(ani_env *env, ani_object obj, ani_string type, ani_int pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, type));
    params.push_back(pattern);
    return createOn(env, obj, params, "On.type");
}

static ani_ref hint(ani_env *env, ani_object obj, ani_string text, ani_int pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    params.push_back(pattern);
    return createOn(env, obj, params, "On.hint");
}

static ani_ref description(ani_env *env, ani_object obj, ani_string text, ani_int pattern) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, text));
    params.push_back(pattern);
    return createOn(env, obj, params, "On.description");
}
static ani_ref inWindow(ani_env *env, ani_object obj, ani_string bundleName) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(aniStringToStdString(env, bundleName));
    return createOn(env, obj, params, "On.inWindow");
}
static ani_ref enabled(ani_env *env, ani_object obj, ani_boolean b) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(static_cast<bool>(b));
    return createOn(env, obj, params, "On.enabled");
}
static ani_ref focused(ani_env *env, ani_object obj, ani_boolean b) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(static_cast<bool>(b));
    return createOn(env, obj, params, "On.focused");
}
static ani_ref clickable(ani_env *env, ani_object obj, ani_boolean b) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(static_cast<bool>(b));
    return createOn(env, obj, params, "On.clickable");
}
static ani_ref checked(ani_env *env, ani_object obj, ani_boolean b) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(static_cast<bool>(b));
    return createOn(env, obj, params, "On.checked");
}
static ani_ref checkable(ani_env *env, ani_object obj, ani_boolean b) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(static_cast<bool>(b));
    return createOn(env, obj, params, "On.checkable");
}
static ani_ref longClickable(ani_env *env, ani_object obj, ani_boolean b) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(static_cast<bool>(b));
    return createOn(env, obj, params, "On.longClickable");
}
static ani_ref selected(ani_env *env, ani_object obj, ani_boolean b) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(static_cast<bool>(b));
    return createOn(env, obj, params, "On.selected");
}
static ani_ref scrollable(ani_env *env, ani_object obj, ani_boolean b) {
    nlohmann::json params = nlohmann::json::array();
    params.push_back(static_cast<bool>(b));
    return createOn(env, obj, params, "On.scrollable");
}

static ani_boolean BindOn(ani_env *env) {
    static const char *className = "Luitest_ani/On;";
    ani_class cls = findCls(env, className);
    if (cls == nullptr) {
        return false;
    }
    std::array methods = {
        ani_native_function {"id", nullptr, reinterpret_cast<void*>(id)},
        ani_native_function {"text", nullptr, reinterpret_cast<void*>(text)},
        ani_native_function {"type", nullptr, reinterpret_cast<void*>(type)},
        ani_native_function {"hint", nullptr, reinterpret_cast<void*>(hint)},
        ani_native_function {"description", nullptr, reinterpret_cast<void*>(description)},
        ani_native_function {"inwindow", nullptr, reinterpret_cast<void*>(inwindow)},
        ani_native_function {"enabled", nullptr, reinterpret_cast<void*>(enabled)},
        ani_native_function {"within", nullptr, reinterpret_cast<void*>(within)},
        ani_native_function {"selected", nullptr, reinterpret_cast<void*>(selected)},
        ani_native_function {"focused", nullptr, reinterpret_cast<void*>(focused)},
        ani_native_function {"clickable", nullptr, reinterpret_cast<void*>(clickable)},
        ani_native_function {"checkable", nullptr, reinterpret_cast<void*>(checkable)},
        ani_native_function {"checked", nullptr, reinterpret_cast<void*>(checked)},
        ani_native_function {"scrollable", nullptr, reinterpret_cast<void*>(scrollable)},
        ani_native_function {"isBefore", nullptr, reinterpret_cast<void*>(isBefore)},
        ani_native_function {"isAfter", nullptr, reinterpret_cast<void*>(isAfter)},
        ani_native_function {"longClickable", nullptr, reinterpret_cast<void*>(longClickable)},
    }
    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        std::cerr << "Cannot bind native methods to " << className << sts::endl;
        return false;
    }
    return true;
}

static ani_ref create([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_class clazz) {
    static const char *className = "Luitest_ani/Driver;";
    ani_class cls;
    ani_ref nullref;
    env->GetNull(&nullref);
    if (ANI_OK != env->FindClass(className, &cls)) {
        return nullref;
    }
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", "Lstd/core/String;:v", &ctor)) {
        return nullref;
    }
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.Create";
    g_apiTransactClient.Transact(callInfo_, reply_);

    ani_ref nativeDriver = UnmarshalReply(env, callInfo_, reply_);
    ani_object driver_object;
    if (ANI_OK != env->Object_New(cls, ctor, &driver_object, reinterpret_cast<ani_object>(nativeDriver))) {
        std::cerr << "New Driver fail" << std::endl;
    }
    return driver_object;
}

static ani_boolean delayMsSync(ani_env *env, ani_object obj, ani_int t) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.Create";
    callInfo_.paramList_.push_back(t);
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref findComponentSync(ani_env *env, ani_object obj, ani_object on_obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.findComponent";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref nativeComponent = UnmarshalReply(env, callInfo_, reply_);
    ani_object com_obj;
    static const char *className = "Luitest_ani/Component;";
    ani_class cls = findCls(env, className);
    ani_method ctor;
    if (cls) {
        ctor = findCtorMethod(env, cls, "Lstd/core/String;:V");
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &com_obj, reinterpret_cast<ani_object>nativeComponent)) {
        std::cerr << "New Component Failed" <<std::endl;
    }
    return com_obj;
}

static ani_object findComponentsSync(ani_env *env, ani_object obj, ani_object on_obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.findComponent";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_object nativeComponents = reinterpret_cast<ani_object>(UnmarshalReply(env, callInfo_, reply_));
    ani_object com_obj;
    ani_object com_objs;
    ani_class arrayCls = findCls(env, "Lescompat/Array");
    ani_method ctor;
    if (arrayCls) {
        ctor = findCtorMethod(env, arrayCls, "I:V");
    }
    ani_size com_size = reply_.resultValue_.size();
    if (ANI_OK!= env->Object_New(arrayCls, ctor, &com_objs, com_size)) {
        std::cout<<"Object_New array failed" << std::endl;
        return com_objs;
    }
    static const char *className = "Luitest_ani/Component;";
    ani_class cls = findCls(env, className);
    ani_method com_ctor;
    if (cls) {
        com_ctor = findCtorMethod(env, cls, "Lstd/core/String;:V");
    }
    if (cls == nullptr || com_ctor == nullptr) {
        return nullptr;
    }
    for (ani_size i = 0; i < com_size; i++) {
        ani_ref item;
        if (ANI_OK != env->Object_New(cls, com_ctor, &com_obj, reinterpret_cast<ani_object>(item))) {
            std::cerr<<"Object new failed : component" <<std::endl;
        }
        if (ANI_OK != env->Object_CallMethodByName_Void(com_objs, "$_set", "ILstd/core/Object;:V", i, com_obj)) {
            std::cerr<<"Object callmethod by name failed :set i" << std::endl;
            break;
        }
    }
    return com_objs;
}

static ani_boolean assertComponentExistSync(ani_env *env, ani_object obj, ani_object on_obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Driver.assertComponentExist";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on_obj, "nativeOn")));
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static void performClick(ani_env *env, ani_object obj, ani_int x, ani_int y, string api) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = api;
    callInfo_.paramList_.push_back(x);
    callInfo_.paramList_.push_back(y);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
}

static ani_boolean clickSync(ani_env *env, ani_object obj, ani_int x, ani_int y) {
    string api_name = "Driver.click";
    performClick(env, obj, x, y, api_name);
    return true;
}

static ani_boolean doubleClickSync(ani_env *env, ani_object obj, ani_int x, ani_int y) {
    string api_name = "Driver.doubleClick";
    performClick(env, obj, x, y, api_name);
    return true;
}

static ani_boolean longClickSync(ani_env *env, ani_object obj, ani_int x, ani_int y) {
    string api_name = "Driver.longClick";
    performClick(env, obj, x, y, api_name);
    return true;
}

static void performDriver(ani_env *env, ani_object obj, string api) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = api;
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
}

static ani_boolean pressBackSync(ani_env *env, ani_object obj, ani_int x, ani_int y) {
    performClick(env, obj, "Driver.pressBack");
    return true;
}

static ani_boolean flingSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_int stepLen, ani_int speed) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.fling";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    callInfo_.paramList_.push_back(stepLen);
    callInfo_.paramList_.push_back(speed);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean inputTextSync(ani_env *env, ani_object obj, ani_object p, ani_string text)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.inputText";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(text);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean swipeSync(ani_env *env, ani_object obj, ani_int x1, ani_int y1, ani_int x2, ani_int y2, ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.swipe";
    callInfo_.paramList_.push_back(x1);
    callInfo_.paramList_.push_back(y1);
    callInfo_.paramList_.push_back(x2);
    callInfo_.paramList_.push_back(y2);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean dragSync(ani_env *env, ani_object obj, ani_int x1, ani_int y1, ani_int x2, ani_int y2, ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.drag";
    callInfo_.paramList_.push_back(x1);
    callInfo_.paramList_.push_back(y1);
    callInfo_.paramList_.push_back(x2);
    callInfo_.paramList_.push_back(y2);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static json getWindowFilter(ani_env *env, ani_object f)
{
    auto filter = json();
    static const char *className = "Luitest_ani/WindowFilterInner;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        return filter;
    }
    
    string list[] = { "bundleName", "title", "focused", "active" };
    for (int i = 0; i < 4; i++) {
        char *cstr = new char[list[i].length() + 1];
        strcpy(cstr, list[i].c_str());
        if (i < 2) {
          ani_ref value;
          if (env->Object_GetPropertyByName_Ref(f, cstr, &value) != ANI_OK) {
              continue;
          }
          if (env->Reference_IsUndefined(value, &ret) != ANI_OK) {
              filter[list[i]] = aniStringToStdString(env, reinterpret_cast<ain_string>(value));
          }
        } else {
            ani_boolean value;
            if (env->Object_GetPropertyByName_Boolen(f, cstr, &value) == ANI_OK) {
              filter[list[i]] = value;
            }
        }
    }
}

static ani_object findWindowSync(ani_env *env, ani_object obj, ani_object filter)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.findWindow";
    callInfo_.paramList_.push_back(getWindowFilter(env, filter));
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref nativeWindow = UnmarshalReply(env, callInfo_, reply_);
    ani_object window_obj;
    static const char *className = "Luitest_ani/UiWindow";
    ani_class cls = findCls(env, className);
    ani_method ctor;
    if (cls) {
        static const char *name = "Lstd/core/String;:v";
        ctor = findCtorMethod(env, cls, name);
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &window_obj, reinterpret_cast<ani_object>(nativeWindow))) {
        std::cerr << "New UiWindow fail" << std::endl;
    }
    return window_obj;
}

static ani_boolean injectMultiPointerActionSync(ani_env *env, ani_object obj, ani_object pointers, ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.injectMultiPointerAction";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, pointers, "nativeDriver")));
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean triggerKeySync(ani_env *env, ani_object obj, ani_int keyCode)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.triggerKey";
    callInfo_.paramList_.push_back(keyCode);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean wakeupDisplaySync(ani_env *env, ani_object obj)
{
    performDriver(env, obj, "Driver.wakeUpDisplay");
    return true;
}

static ani_boolean pressHomeSync(ani_env *env, ani_object obj)
{
    performDriver(env, obj, "Driver.pressHome");
    return true;
}

static ani_object getDisplaySizeSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplaySize";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"]);
    return p;
}

static ani_object getDisplaySizeDensitySync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplaySizeDensity";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"]);
    return p;
}

static ani_int getDisplayRotationSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.getDisplayRotation";
    g_apiTransactClient.Transact(callInfo_, reply_);
    return UnmarshalReply(env, callInfo_, reply_);
}

static ani_boolean waitForIdleSync(ani_env *env, ani_object obj, ani_int idleTime, ani_int timeout)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.waitForIdle";
    callInfo_.paramList_.push_back(idleTime);
    callInfo_.paramList_.push_back(timeout);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_object waitForComponentSync(ani_env *env, ani_object obj, ani_object on_obj, ani_int time)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.waitForComponent";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, obj, "nativeOn")));
    callInfo_.paramList_.push_back(time);
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref nativeComponent = UnmarshalReply(env, callInfo_, reply_);
    ani_object component_obj;
    static const char *className = "Luitest_ani/Component";
    ani_class cls = findCls(env, className);
    ani_method ctor;
    if (cls) {
        static const char *name = "Lstd/core/String;:v";
        ctor = findCtorMethod(env, cls, name);
    }
    if (cls == nullptr || ctor == nullptr) {
        return nullptr;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &component_obj, reinterpret_cast<ani_object>(nativeComponent))) {
        std::cerr << "New Component fail" << std::endl;
    }
    return component_obj;
}

static ani_boolean triggerCombineKeysSync(ani_env *env, ani_object obj, ani_int key0, ani_int key1, ani_int key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.triggerCombineKeys";
    callInfo_.paramList_.push_back(key0);
    callInfo_.paramList_.push_back(key1);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key2), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(key2);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean setDisplayRotationSync(ani_env *env, ani_object obj, ani_int rotation)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.setDisplayRotation";
    callInfo_.paramList_.push_back(rotation);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean screenCaptureSync(ani_env *env, ani_object obj, ani_string path, ani_object rect)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.screenCapture";
    string savePath = aniStringToStdString(env, path);
    auto fd = open(savePath.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return false;
    }
    callInfo_.paramList_.push_back(fd);
    // 咋取rect
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean screenCapSync(ani_env *env, ani_object obj, ani_string path, ani_object rect)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.screenCap";
    string savePath = aniStringToStdString(env, path);
    auto fd = open(savePath.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return false;
    }
    callInfo_.paramList_.push_back(fd);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean setDisplayRotationEnabledSync(ani_env *env, ani_object obj, ani_boolean enable)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.setDisplayRotationEnabled";
    callInfo_.paramList_.push_back(enable);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean penSwipeSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_int speed, ani_int pressure)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.penSwipe";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(pressure), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(pressure);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean penClickSync(ani_env *env, ani_object obj, ani_object p)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.penClick";
    auto point = getPoint(env, f);
    callInfo_.paramList_.push_back(point);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean penDoubleClickSync(ani_env *env, ani_object obj, ani_object p)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.penDoubleClick";
    auto point = getPoint(env, f);
    callInfo_.paramList_.push_back(point);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean penDoubleClickSync(ani_env *env, ani_object obj, ani_object p, ani_int pressure)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.penDoubleClick";
    auto point = getPoint(env, f);
    callInfo_.paramList_.push_back(point);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(pressure), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(pressure);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseScrollSync(ani_env *env, ani_object obj, ani_object p, ani_boolean down, ani_int dis, ani_int key1, ani_int key2,
    ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseScroll";
    auto point = getPoint(env, p);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(down);
    callInfo_.paramList_.push_back(dis);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key1), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(key1);
    }
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key2), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(key2);
    }
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseMoveWithTrackSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseMoveWithTrack";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseDragkSync(ani_env *env, ani_object obj, ani_object f, ani_object t, ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseDragk";
    auto from = getPoint(env, f);
    auto to = getPoint(env, t);
    callInfo_.paramList_.push_back(from);
    callInfo_.paramList_.push_back(to);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseMoveToSync(ani_env *env, ani_object obj, ani_object p)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseMoveTo";
    auto point = getPoint(env, f);
    callInfo_.paramList_.push_back(point);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseClickSync(ani_env *env, ani_object obj, ani_object p, ani_int btnId, ani_int key1, ani_int key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseClickSync";
    auto point = getPoint(env, f);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(btnId);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key1), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
        if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key2), &ret) != ANI_OK) {
            callInfo_.paramList_.push_back(key2);
        }
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseDoubleClickSync(ani_env *env, ani_object obj, ani_object p, ani_int btnId, ani_int key1, ani_int key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseDoubleClick";
    auto point = getPoint(env, f);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(btnId);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key1), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
        if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key2), &ret) != ANI_OK) {
            callInfo_.paramList_.push_back(key2);
        }
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean mouseLongClickSync(ani_env *env, ani_object obj, ani_object p, ani_int btnId, ani_int key1, ani_int key2)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.mouseLongClick";
    auto point = getPoint(env, f);
    callInfo_.paramList_.push_back(point);
    callInfo_.paramList_.push_back(btnId);
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key1), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(key1);
        if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(key2), &ret) != ANI_OK) {
            callInfo_.paramList_.push_back(key2);
        }
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean injectMultiPointerActionSync(ani_env *env, ani_object obj, ani_object pointers, ani_int speed)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.injectMultiPointerAction";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, obj, "nativePointerMatrix")));
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean injectPenPointerActionSync(ani_env *env, ani_object obj, ani_object pointers, ani_int speed, ani_int pressure)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.injectPenPointerAction";
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, obj, "nativePointerMatrix")));
    ani_boolean ret = false;
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(speed), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(speed);
    }
    if (env->Reference_IsUndefined(reinterpret_cast<ani_ref>(pressure), &ret) != ANI_OK) {
        callInfo_.paramList_.push_back(pressure);
    }
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}


static json getTouchPadSwipeOptions(ani_env *env, ani_object f)
{
    auto filter = json();
    static const char *className = "Luitest_ani/TouchPadSwipeOptionsInner;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        return filter;
    }
    
    string list[] = { "stay", "speed" };
    for (int i = 0; i < 2; i++) {
        char *cstr = new char[list[i].length() + 1];
        strcpy(cstr, list[i].c_str());
        if (i == 1) {
          ani_int value;
          if (env->Object_GetPropertyByName_Int(f, cstr, &value) != ANI_OK) {
              filter[list[i]] = value;
              continue;
          }
        } else {
            ani_boolean value;
            if (env->Object_GetPropertyByName_Boolen(f, cstr, &value) == ANI_OK) {
              filter[list[i]] = value;
            }
        }
    }
}

static ani_boolean touchPadMultiFingerSwipeSync(ani_env *env, ani_object obj, ani_int fingers, ani_int direction, ani_object touchPadOpt)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeDriver"));
    callInfo_.apiId_ = "Driver.touchPadMultiFingerSwipe";
    callInfo_.paramList_.push_back(fingers);
    callInfo_.paramList_.push_back(direction);
    callInfo_.paramList_.push_back(getTouchPadSwipeOptions(env, touchPadOpt));
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}


static ani_boolean BindDriver(ani_env *env)
{
    static const char *className = "Luitest_ani/Driver;";
    ani_class cls;
        if (ANI_OK != env->FindClass(className, &cls)) {
        return false;
    }

    std::array methods = {
        ani_native_function {"create", "Luitest_ani/Driver;", reinterpret_cast<void *>(create)},
        ani_native_function {"delayMsSync", "I:I", reinterpret_cast<void *>(delayMsSync)},
        ani_native_function {"clickSync", nullptr, reinterpret_cast<void *>(clickSync)},
        ani_native_function {"longClickSync", nullptr, reinterpret_cast<void *>(longClickSync)},
        ani_native_function {"doubleClickSync", nullptr, reinterpret_cast<void *>(doubleClickSync)},
        ani_native_function {"flingSync", nullptr, reinterpret_cast<void *>(flingSync)},
        ani_native_function {"swipeSync", nullptr, reinterpret_cast<void *>(swipeSync)},
        ani_native_function {"dragSync", nullptr, reinterpret_cast<void *>(dragSync)},
        ani_native_function {"pressBackSync", nullptr, reinterpret_cast<void *>(pressBackSync)},
        ani_native_function {"assertComponentExistSync", nullptr, reinterpret_cast<void *>(assertComponentExistSync)},
        ani_native_function {"triggerKeySync", nullptr, reinterpret_cast<void *>(triggerKeySync)},
        ani_native_function {"inputTextSync", nullptr, reinterpret_cast<void *>(inputTextSync)},
        ani_native_function {"findWindowSync", nullptr, reinterpret_cast<void *>(findWindowSync)},
        ani_native_function {"wakeUpDisplaySync", nullptr, reinterpret_cast<void *>(wakeUpDisplaySync)},
        ani_native_function {"pressHomeSync", nullptr, reinterpret_cast<void *>(pressHomeSync)},
        ani_native_function {"getDisplaySizeSync", nullptr, reinterpret_cast<void *>(getDisplaySizeSync)},
        ani_native_function {"getDisplaySizeDensitySync", nullptr, reinterpret_cast<void *>(getDisplaySizeDensitySync)},
        ani_native_function {"getDisplayRotationSync", nullptr, reinterpret_cast<void *>(getDisplayRotationSync)},
        ani_native_function {"findComponentsSync", nullptr, reinterpret_cast<void *>(findComponentsSync)},
        ani_native_function {"waitForIdleSync", nullptr, reinterpret_cast<void *>(waitForIdleSync)},
        ani_native_function {"waitForComponentSync", nullptr, reinterpret_cast<void *>(waitForComponentSync)},
        ani_native_function {"triggerCombineKeysSync", nullptr, reinterpret_cast<void *>(triggerCombineKeysSync)},
        ani_native_function {"setDisplayRotationEnabledSync", nullptr, reinterpret_cast<void *>(setDisplayRotationEnabledSync)},
        ani_native_function {"setDisplayRotationSync", nullptr, reinterpret_cast<void *>(setDisplayRotationSync)},
        ani_native_function {"screenCaptureSync", nullptr, reinterpret_cast<void *>(screenCaptureSync)},
        ani_native_function {"screenCapSync", nullptr, reinterpret_cast<void *>(screenCapSync)},
        ani_native_function {"penSwipeSync", nullptr, reinterpret_cast<void *>(penSwipeSync)},
        ani_native_function {"penClickSync", nullptr, reinterpret_cast<void *>(penClickSync)},
        ani_native_function {"penDoubleClickSync", nullptr, reinterpret_cast<void *>(penDoubleClickSync)},
        ani_native_function {"penLongClickSync", nullptr, reinterpret_cast<void *>(penLongClickSync)},
        ani_native_function {"mouseScrollSync", nullptr, reinterpret_cast<void *>(mouseScrollSync)},
        ani_native_function {"mouseMoveWithTrackSync", nullptr, reinterpret_cast<void *>(mouseMoveWithTrackSync)},
        ani_native_function {"mouseMoveToSync", nullptr, reinterpret_cast<void *>(mouseMoveToSync)},
        ani_native_function {"mouseDragkSync", nullptr, reinterpret_cast<void *>(mouseDragkSync)},
        ani_native_function {"mouseClickSync", nullptr, reinterpret_cast<void *>(mouseClickSync)},
        ani_native_function {"mouseDoubleClickSync", nullptr, reinterpret_cast<void *>(mouseDoubleClickSync)},
        ani_native_function {"mouseLongClickSync", nullptr, reinterpret_cast<void *>(mouseLongClickSync)},
        ani_native_function {"injectMultiPointerActionSync", nullptr, reinterpret_cast<void *>(injectMultiPointerActionSync)},
        ani_native_function {"injectPenPointerActionSync", nullptr, reinterpret_cast<void *>(injectPenPointerActionSync)},
    }

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        std::cerr << "Cannot bind native methods to " << className <<" " <<std::endl;
        return false;
    }
    return true;
}


static void performWindow(ani_env *env, ani_object obj, string api)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = api;
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
}

static boolen splitSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.split");
    return true;
}

static boolen resumeSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.resume");
    return true;
}

static boolen closeSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.close");
    return true;
}

static boolen minimizeSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.minimize");
    return true;
}

static boolen maximizeSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.maximize");
    return true;
}

static boolen focusSync(ani_env *env, ani_object obj, string api)
{
    performWindow(env, obj, "UiWindow.focus");
    return true;
}

static ani_ref isFocusedSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.isFocused";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref isActiveSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.isActive";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_boolean resizeSync(ani_env *env, ani_object obj, ani_int w, ani_int h, ani_int d)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.resize";
    callInfo_.paramList_.push_back(w);
    callInfo_.paramList_.push_back(h);
    callInfo_.paramList_.push_back(d);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean moveToSync(ani_env *env, ani_object obj, ani_int x, ani_int y)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.moveTo";
    callInfo_.paramList_.push_back(x);
    callInfo_.paramList_.push_back(y);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref getWindowModeSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getWindowMode";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref GetBundleNameSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.GetBundleName";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref getTitileSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getTitile";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref ret =  UnmarshalReply(env, callInfo_, reply_);
    return ret;
}

static ani_ref getBoundsSync(ani_env *env, ani_object obj)
{
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeWindow"));
    callInfo_.apiId_ = "UiWindow.getBounds";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_object r = newRect(env, obj, reply_.resultValue_);
    return r;
}

static ani_boolean BindWindow(ani_env *env)
{
    static const char *className = "Luitest_ani/UiWindow;";
    ani_class cls;
        if (ANI_OK != env->FindClass(className, &cls)) {
        return false;
    }

    std::array methods = {
        ani_native_function {"splitSync", nullptr, reinterpret_cast<void *>(splitSync)},
        ani_native_function {"resumeSync", nullptr, reinterpret_cast<void *>(resumeSync)},
        ani_native_function {"closeSync", nullptr, reinterpret_cast<void *>(closeSync)},
        ani_native_function {"minimizeSync", nullptr, reinterpret_cast<void *>(minimizeSync)},
        // ani_native_function {"maximizeSync", nullptr, reinterpret_cast<void *>(maximizeSync)},
        // ani_native_function {"focusSync", nullptr, reinterpret_cast<void *>(focusSync)},
        ani_native_function {"isFocusedSync", nullptr, reinterpret_cast<void *>(isFocusedSync)},
        ani_native_function {"isActiveSync", nullptr, reinterpret_cast<void *>(isActiveSync)},
        ani_native_function {"resizeSync", nullptr, reinterpret_cast<void *>(resizeSync)},
        ani_native_function {"moveToSync", nullptr, reinterpret_cast<void *>(moveToSync)},
        ani_native_function {"getWindowModeSync", nullptr, reinterpret_cast<void *>(getWindowModeSync)},
        ani_native_function {"GetBundleNameSync", nullptr, reinterpret_cast<void *>(GetBundleNameSync)},
        ani_native_function {"getTitileSync", nullptr, reinterpret_cast<void *>(getTitileSync)},
        ani_native_function {"getBoundsSync", nullptr, reinterpret_cast<void *>(getBoundsSync)},
    }

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        std::cerr << "Cannot bind native methods to " << className <<" " <<std::endl;
        return false;
    }
    return true;
}

static ani_ref getBoundsCenterSync(ani_env *env, ani_object obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.apiId_ = "Component.getBoundsCenter";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_object p = newPoint(env, obj, reply_.resultValue_["x"], reply_.resultValue_["y"])
    return p;
}
static ani_ref getBoundsSync(ani_env *env, ani_object obj) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.apiId_ = "Component.getBounds";
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_object r = newRect(env, obj, reply_.resultValue_);
    return r;
}

static ani_ref performComponentApi(ani_env *env, ani_object obj, string apiId) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId = apiId;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    g_apiTransactClient.Transact(callInfo_, reply_);
    ani_ref result = UnmarshalReply(env, callInfo_, reply_);
    return result;
}

static ani_boolean comClick(ani_env *env, ani_object obj) {
    performComponentApi(obj, "Component.click");
    return true;
}

static ani_boolean comDoubleClick(ani_env *env, ani_object obj) {
    performComponentApi(obj, "Component.doubleClick");
    return true;
}

static ani_boolean comLongClick(ani_env *env, ani_object obj) {
    performComponentApi(obj, "Component.longClick");
    return true;
}

static ani_boolean comDragToSync(ani_env *env, ani_object obj, ani_object target) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId = "Component.dragTo";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, target, "nativeComponent")))；
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_ref getText(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getText")
}

static ani_ref getType(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getType")
}

static ani_ref getId(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getId")
}

static ani_ref getHint(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getHint")
}

static ani_ref getDescription(ani_env *env, ani_object obj) {
    return performComponentApi(env, obj, "Component.getDescription")
}

static ani_boolean comInputText(ani_env *env, ani_object obj, ani_string txt) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.inputText";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, txt));
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean clearText(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.clearText");
    return true;
}

static ani_boolean scrollToTop(ani_env *env, ani_object obj, ani_double speed) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.scrollToTop";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(speed);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean scrollToBottom(ani_env *env, ani_object obj, ani_double speed) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId_ = "Component.scrollToBottom";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(speed);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static ani_boolean scrollSearch(ani_env *env, ani_object obj, ani_object on, ani_boolean vertical, ani_double offset) {
    ApiCallInfo callInfo_;
    ApiCallInfo reply_;
    callInfo_.apiId = "Component.scrollSearch";
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(aniStringToStdString(env, unwrapp(env, on, "nativeComponent")));
    callInfo_.paramList_.push_back(reinterpret_cast<bool>(vertical));
    callInfo_.paramList_.push_back(offset);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return true;
}

static void pinch(ani_env *env, ani_object obj, ani_double scale, string apiId) {
    ApiCallInfo callInfo_;
    ApiReplyInfo reply_;
    callInfo_.apiId = apiId;
    callInfo_.callerObjRef_ = aniStringToStdString(env, unwrapp(env, obj, "nativeComponent"));
    callInfo_.paramList_.push_back(scale);
    g_apiTransactClient.Transact(callInfo_, reply_);
    UnmarshalReply(env, callInfo_, reply_);
    return;
}

static ani_boolean pinchIn(ani_env *env, ani_object obj, ani_double scale) {
    pinch(env, obj, scale, "Component.pinchIn");
    return true;
}

static ani_boolean pinchOut(ani_env *env, ani_object obj, ani_double scale) {
    pinch(env, obj, scale, "Component.pinchOut");
    return true;
}

static ani_boolean isSelected(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.isSelected");
    return true;
}

static ani_boolean isClickable(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.isClickable");
    return true;
}
static ani_boolean isLongClickable(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.isLongClickable");
    return true;
}
static ani_boolean isScrollable(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.isScrollable");
    return true;
}

static ani_boolean isEnabled(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.isEnabled");
    return true;
}
static ani_boolean isFocused(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.isFocused");
    return true;
}
static ani_boolean isChecked(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.isChecked");
    return true;
}
static ani_boolean isCheckable(ani_env *env, ani_object obj) {
    performComponentApi(env, obj, "Component.isCheckable");
    return true;
}
static ani_boolean BindComponent(ani_env *env) {
    static const char *className = "Luitest_ani/Component";
   ani_class cls;
        if (ANI_OK != env->FindClass(className, &cls)) {
        return false;
    }
    std::array methods = {
        ani_native_function {"comClickSync", nullptr, reinterpret_cast<void *>(comClick)},
        ani_native_function {"comLongClickSync", nullptr, reinterpret_cast<void *>(comLongClick)},
        ani_native_function {"comDoubleClickSync", nullptr, reinterpret_cast<void *>(comDoubleClick)},
        ani_native_function {"comDragToSync", nullptr, reinterpret_cast<void *>(comDragToSync)},
        ani_native_function {"getBoundsSync", nullptr, reinterpret_cast<void *>(getBounds)},
        ani_native_function {"getBoundsCenterSync", nullptr, reinterpret_cast<void *>(getBoundsCenter)},
        ani_native_function {"getTextSync", nullptr, reinterpret_cast<void *>(getText)},
        ani_native_function {"getTypeSync", nullptr, reinterpret_cast<void *>(getType)},
        ani_native_function {"getIdSync", nullptr, reinterpret_cast<void *>(getId)},
        ani_native_function {"getHintSync", nullptr, reinterpret_cast<void *>(getHint)},
        ani_native_function {"getDescriptionSync", nullptr, reinterpret_cast<void *>(getDescription)},
        ani_native_function {"comInputTextSync", nullptr, reinterpret_cast<void *>(comInputText)},
        ani_native_function {"clearTextSync", nullptr, reinterpret_cast<void *>(clearText)},
        ani_native_function {"scrollToTopSync", nullptr, reinterpret_cast<void *>(scrollToTop)},
        ani_native_function {"scrollToBottomSync", nullptr, reinterpret_cast<void *>(scrollToBottom)},
        ani_native_function {"scrollSearchSync", nullptr, reinterpret_cast<void *>(scrollSearch)},
        ani_native_function {"pinchInSync", nullptr, reinterpret_cast<void *>(pinchIn)},
        ani_native_function {"pinchOutSync", nullptr, reinterpret_cast<void *>(pinchOut)},
        ani_native_function {"isScrollableSync", nullptr, reinterpret_cast<void *>(isScrollable)},
        ani_native_function {"isSelectedSync", nullptr, reinterpret_cast<void *>(isSelected)},
        ani_native_function {"isLongClickableSync", nullptr, reinterpret_cast<void *>(isLongClickable)},
        ani_native_function {"isClickableSync", nullptr, reinterpret_cast<void *>(isClickable)},
        ani_native_function {"isFocusedSync", nullptr, reinterpret_cast<void *>(isFocused)},
        ani_native_function {"isEnabledSync", nullptr, reinterpret_cast<void *>(isEnabled)},
        ani_native_function {"isCheckedSync", nullptr, reinterpret_cast<void *>(isChecked)},
        ani_native_function {"isCheckableSync", nullptr, reinterpret_cast<void *>(isCheckable)},
    }

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        std::cerr << "Cannot bind native methods to " << className <<" " <<std::endl;
        return false;
    }
    return true;
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result) {
    LOG_D("UITest: ANI_EXPORT Start!");
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        LOG_D("UITest: Unsupported ANI_VERSION_1");
        return (ani_status)ANI_ERROR;
    }
    auto status = true;
    status &= BindDriver(env);
    status &= BindOn(env);
    status &= BindComponent(env);
    status &= BindWindow(env);
    status &= BindInterfaces(env);
    status &= BindPointMatrix(env);
    if (!status) {
        return ANI_ERROR;
    }
    *result = ANI_VERSION_1;
    return ANI_OK;
}
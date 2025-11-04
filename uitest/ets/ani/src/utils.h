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

#ifndef UTILS_H
#define UTILS_H

#include "ani.h"
#include "hilog/log.h"
#include <array>
#include <iostream>
#include "frontend_api_defines.h"
#include <ani_signature_builder.h>

namespace OHOS::uitest {
    using namespace nlohmann;
    using namespace std;
    using namespace OHOS::HiviewDFX;
    using namespace arkts::ani_signature;
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LogType::LOG_CORE, 0xD003100, "UiTestKit"};

    inline ani_class findCls(ani_env *env, const char *className)
    {
        ani_class cls;
        ani_ref nullref;
        env->GetNull(&nullref);
        if (ANI_OK != env->FindClass(className, &cls)) {
            HiLog::Error(LABEL, "Not found className: %{public}s", className);
        }
        return cls;
    }

    inline ani_method findCtorMethod(ani_env *env, ani_class cls, const char *name)
    {
        ani_method ctor = nullptr;
        if (ANI_OK != env->Class_FindMethod(cls, Builder::BuildConstructorName().c_str(), name, &ctor)) {
            HiLog::Error(LABEL, "Not found ctor: %{public}s", name);
        }
        return ctor;
    }

	inline ani_object createInt(ani_env *env, ani_int displayId)
	{
		static constexpr const char *className = "std.core.Int";
		ani_class IntCls {};
		env->FindClass(className, &IntCls);
		ani_method ctor {};
		env->Class_FindMethod(IntCls, "<ctor>", "i:", &ctor);
		ani_object obj {};
		if (env->Object_New(IntCls, ctor, &obj, displayId)!=ANI_OK) {
			ani_ref unRef;
			env->GetUndefined(&unRef);
			return reinterpret_cast<ani_object>(unRef);
		}
		return obj;
	}
	
	inline ani_object newRect(ani_env *env, ani_object object, nlohmann::json num)
	{
	    ani_object rect_obj = {};
	    ani_class cls = findCls(env, Builder::BuildClass({"@ohos", "UiTest", "RectInner"}).Descriptor().c_str());
	    ani_method ctor = nullptr;
	    if (cls != nullptr) {
	        static const char *name = nullptr;
	        ctor = findCtorMethod(env, cls, name);
	    }
	    if (cls == nullptr || ctor == nullptr) {
	        return nullptr;
	    }
	    if (ANI_OK != env->Object_New(cls, ctor, &rect_obj)) {
	        HiLog::Error(LABEL, "Create Rect object failed");
	        return rect_obj;
	    }
	    ani_method setter;
	    string direct[] = {"left", "top", "right", "bottom", "displayId"};
	    for (int index = 0; index < FIVE; index++) {
	        string tag = direct[index];
	        if (index == FOUR) {
	            auto ret1 = env->Object_SetPropertyByName_Ref(rect_obj, "displayId",
					reinterpret_cast<ani_ref>(createInt(env, ani_int(num[tag]))));
				HiLog::Info(LABEL, "Object_SetPropertyByName_Ref status, %{public}d", ret1);
	        } else {
	            char *setter_name = strdup((Builder::BuildSetterName(tag)).c_str());
	            if (ANI_OK != env->Class_FindMethod(cls, setter_name, nullptr, &setter)) {
	                HiLog::Error(LABEL, "Find Method <set>tag failed");
	            }
	            if (ANI_OK != env->Object_CallMethod_Void(rect_obj, setter, ani_int(num[tag]))) {
	                HiLog::Error(LABEL, "call setter failed %{public}s", direct[index].c_str());
	                return rect_obj;
	            }
	        }
	    }
	    return rect_obj;
	}
}

#endif
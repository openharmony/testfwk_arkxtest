/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

#include "ui_driver.h"

// register all the ExternAPI creators and invokers.
namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    /**Create KeyAction object by index.*/
    static unique_ptr<KeyAction> CreateKeyAction(uint32_t index, uint32_t keyCode)
    {
        if (index == UiKey::BACK) {
            return make_unique<Back>();
        } else if (index == UiKey::GENERIC) {
            return make_unique<AnonymousSingleKey>(keyCode);
        } else {
            return nullptr;
        }
    }

    static bool WidgetSelectorHandler(string_view function, json &caller, const json &in, json &out, ApiCallErr &err)
    {
        static const set<string_view> widgetSelectorApis = {
            "WidgetSelector::<init>", "WidgetSelector::AddMatcher",
            "WidgetSelector::AddFrontLocator", "WidgetSelector::AddRearLocator"};
        if (widgetSelectorApis.find(function) == widgetSelectorApis.end()) {
            return false;
        }

        if (function == "WidgetSelector::<init>") {
            auto selector = WidgetSelector();
            PushBackValueItemIntoJson<WidgetSelector>(selector, out);
            return true;
        }

        auto selector = WidgetSelector();
        selector.ReadFromParcel(caller);
        if (function == "WidgetSelector::AddMatcher") {
            auto attrName = GetItemValueFromJson<string>(in, 0);
            auto testValue = GetItemValueFromJson<string>(in, 1);
            auto matchRule = GetItemValueFromJson<uint32_t>(in, 2);
            if (matchRule < EQ || matchRule > ENDS_WITH) {
                err = ApiCallErr(INTERNAL_ERROR, "Illegal match rule: " + to_string(matchRule));
            }
            auto matcher = WidgetAttrMatcher(attrName, testValue, static_cast<ValueMatchRule>(matchRule));
            selector.AddMatcher(matcher);
        } else if (function == "WidgetSelector::AddFrontLocator") {
            auto frontLocator = WidgetSelector();
            frontLocator.ReadFromParcel(GetItemValueFromJson<json>(in, 0));
            selector.AddFrontLocator(frontLocator, err);
        } else if (function == "WidgetSelector::AddRearLocator") {
            auto rearLocator = WidgetSelector();
            rearLocator.ReadFromParcel(GetItemValueFromJson<json>(in, 0));
            selector.AddRearLocator(rearLocator, err);
        }
        // write back updated object meta-data
        caller.clear();
        selector.WriteIntoParcel(caller);
        return true;
    }

    static bool UiDriverHandlerA(string_view function, json &caller, const json &in, json &out, ApiCallErr &err)
    {
        static const set<string_view> uiDriverApis = {
            "UiDriver::<init>", "UiDriver::FindWidgets", "UiDriver::TriggerKey"};
        if (uiDriverApis.find(function) == uiDriverApis.end()) {
            return false;
        }

        if (function == "UiDriver::<init>") {
            string targetDevice = "local";
            if (!in.empty()) {
                targetDevice = GetItemValueFromJson<string>(in, 0);
            }
            auto driver = UiDriver(targetDevice);
            PushBackValueItemIntoJson<UiDriver>(driver, out);
            return true;
        }

        auto driver = UiDriver("");
        driver.ReadFromParcel(caller);
        if (function == "UiDriver::FindWidgets") {
            auto selector = WidgetSelector();
            selector.ReadFromParcel(GetItemValueFromJson<json>(in, 0));
            vector<unique_ptr<WidgetImage>> rev;
            driver.FindWidgets(selector, rev, err);
            for (auto &ptr:rev) {
                PushBackValueItemIntoJson<WidgetImage>(*ptr, out);
            }
        } else if (function == "UiDriver::TriggerKey") {
            const auto keyIndex = GetItemValueFromJson<uint32_t>(in, 0);
            uint32_t keyCode = 0;
            if (in.size() >= INDEX_TWO) {
                keyCode = GetItemValueFromJson<uint32_t>(in, 1);
            }
            unique_ptr<KeyAction> action = CreateKeyAction(keyIndex, keyCode);
            if (action == nullptr) {
                err = ApiCallErr(INTERNAL_ERROR, "No such KeyAction: " + to_string(keyIndex));
            } else {
                driver.TriggerKey(*action, err);
            }
        }
        // write back updated object meta-data
        caller.clear();
        driver.WriteIntoParcel(caller);
        return true;
    }

    static bool UiDriverHandlerB(string_view function, json &caller, const json &in, json &out, ApiCallErr &err)
    {
        static const set<string_view> uiDriverApis = {"UiDriver::PerformWidgetOperate",
            "UiDriver::InputText", "UiDriver::ScrollSearch", "UiDriver::GetWidgetAttribute"};
        if (uiDriverApis.find(function) == uiDriverApis.end()) {
            return false;
        }

        auto driver = UiDriver("");
        driver.ReadFromParcel(caller);
        if (function == "UiDriver::PerformWidgetOperate") {
            auto img = WidgetImage();
            img.ReadFromParcel(GetItemValueFromJson<json>(in, 0));
            const auto opType = static_cast<WidgetOp>(GetItemValueFromJson<uint32_t>(in, 1));
            driver.PerformWidgetOperate(img, opType, err);
        } else if (function == "UiDriver::InputText") {
            auto img = WidgetImage();
            img.ReadFromParcel(GetItemValueFromJson<json>(in, 0));
            const auto text = GetItemValueFromJson<string>(in, 1);
            driver.InputText(img, text, err);
        } else if (function == "UiDriver::ScrollSearch") {
            auto img = WidgetImage();
            auto selector = WidgetSelector();
            img.ReadFromParcel(GetItemValueFromJson<json>(in, 0));
            selector.ReadFromParcel(GetItemValueFromJson<json>(in, 1));
            static constexpr int32_t scrollDeadZone = 20;
            auto imgPtr = driver.ScrollSearch(img, selector, err, scrollDeadZone);
            if (imgPtr != nullptr) {
                PushBackValueItemIntoJson(*imgPtr, out);
            }
        } else if (function == "UiDriver::GetWidgetAttribute") {
            auto img = WidgetImage();
            img.ReadFromParcel(GetItemValueFromJson<json>(in, 0));
            auto attrName = GetItemValueFromJson<string>(in, 1);
            // update widget and then get attribute
            driver.UpdateWidgetImage(img, err);
            auto attrValue = img.GetAttribute(attrName, "null");
            if (attrValue != "null") {
                PushBackValueItemIntoJson<string>(attrValue, out);
            }
        }
        // write back updated object meta-data
        caller.clear();
        driver.WriteIntoParcel(caller);
        return true;
    }

    static bool UiDriverHandlerC(string_view function, json &caller, const json &in, json &out, ApiCallErr &err)
    {
        static const set<string_view> uiDriverApis = {"UiDriver::PerformGenericClick", "UiDriver::PerformGenericSwipe",
            "UiDriver::DragWidgetToAnother", "UiDriver::TakeScreenCap", "UiDriver::DelayMs"};
        if (uiDriverApis.find(function) == uiDriverApis.end()) {
            return false;
        }

        auto driver = UiDriver("");
        driver.ReadFromParcel(caller);
        if (function == "UiDriver::PerformGenericClick") {
            const auto actionType = static_cast<PointerOp>(GetItemValueFromJson<uint32_t>(in, 0));
            auto px = GetItemValueFromJson<int32_t>(in, 1);
            auto py = GetItemValueFromJson<int32_t>(in, 2);
            driver.PerformGenericClick(actionType, {px, py}, err);
        } else if (function == "UiDriver::PerformGenericSwipe") {
            const auto actionType = static_cast<PointerOp>(GetItemValueFromJson<uint32_t>(in, 0));
            auto px0 = GetItemValueFromJson<int32_t>(in, 1);
            auto py0 = GetItemValueFromJson<int32_t>(in, 2);
            auto px1 = GetItemValueFromJson<int32_t>(in, 3);
            auto py1 = GetItemValueFromJson<int32_t>(in, 4);
            driver.PerformGenericSwipe(actionType, {px0, py0}, {px1, py1}, err);
        } else if (function == "UiDriver::DragWidgetToAnother") {
            auto img0 = WidgetImage();
            img0.ReadFromParcel(GetItemValueFromJson<json>(in, 0));
            auto img1 = WidgetImage();
            img1.ReadFromParcel(GetItemValueFromJson<json>(in, 1));
            driver.DragWidgetToAnother(img0, img1, err);
        } else if (function == "UiDriver::TakeScreenCap") {
            driver.TakeScreenCap(GetItemValueFromJson<string>(in, 0), err);
            PushBackValueItemIntoJson<bool>(err.code_ == NO_ERROR, out);
        } else if (function == "UiDriver::DelayMs") {
            UiDriver::DelayMs(GetItemValueFromJson<uint32_t>(in, 0));
        }
        // write back updated object meta-data
        caller.clear();
        driver.WriteIntoParcel(caller);
        return true;
    }

    void RegisterExternApIs()
    {
        auto &server = ExternApiServer::Get();
        server.AddHandler(WidgetSelectorHandler);
        server.AddHandler(UiDriverHandlerA);
        server.AddHandler(UiDriverHandlerB);
        server.AddHandler(UiDriverHandlerC);
    }
}
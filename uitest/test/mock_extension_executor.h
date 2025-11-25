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
#ifndef MOCK_EXTENSION_EXECUTOR_H
#define MOCK_EXTENSION_EXECUTOR_H

#include <csignal>
#include <dlfcn.h>
#include <mutex>
#include <securec.h>
#include <set>
#include <string_view>
#include "common_utilities_hpp.h"
#include "frontend_api_handler.h"
#include "ui_action.h"
#include "ui_driver.h"
#include "ui_record.h"
#include "screen_copy.h"
#include "extension_c_api.h"

namespace OHOS::uitest {
    using namespace std;
    constexpr auto ERR_BAD_ARG = ErrCode::ERR_INVALID_INPUT;
    constexpr LogType type = LogType::LOG_APP;
    string g_lastErrorMessage = "";
    int32_t g_lastErrorCode = 0;
    mutex g_captureLock;
    mutex g_recordRunningLock;
    set<string> g_runningCaptures;

class MockDriver : public UiDriver {
public:
    MockDriver() : UiDriver() {};
    ~MockDriver() = default;

    void PerformMouseAction(const MouseAction &touch, const UiOpArgs &opt, ApiCallErr &err) {}
    void PerformTouch(const TouchAction &touch, const UiOpArgs &opt, ApiCallErr &err) {}
    void DumpUiHierarchy(nlohmann::json &out, const DumpOption &option, ApiCallErr &error) {}
};

bool ExtensionApiCheck(bool cond)
{
    if (cond) {
        return true;
    }
    return false;
}

RetCode AtomicMouseAction(int32_t stage, int32_t px, int32_t py, int32_t btn)
{
    static auto driver = MockDriver();
    if (!ExtensionApiCheck(stage >= ActionStage::DOWN && stage <= ActionStage::AXIS_STOP)) {
        return RETCODE_FAIL;
    }
    if (!ExtensionApiCheck(btn >= MouseButton::BUTTON_NONE && btn <= MouseButton::BUTTON_MIDDLE)) {
        return RETCODE_FAIL;
    }
    auto touch = GenericAtomicMouseAction(static_cast<ActionStage>(stage), Point(px, py),
                                          static_cast<MouseButton>(btn));
    auto err = ApiCallErr(NO_ERROR);
    UiOpArgs uiOpArgs;
    driver.PerformMouseAction(touch, uiOpArgs, err);
    if (!ExtensionApiCheck(err.code_ == NO_ERROR)) {
        return RETCODE_FAIL;
    }
    return RETCODE_SUCCESS;
}

RetCode AtomicTouch(int32_t stage, int32_t px, int32_t py)
{
    static auto driver = MockDriver();
    if (!ExtensionApiCheck(stage >= ActionStage::DOWN && stage <= ActionStage::UP)) {
        return RETCODE_FAIL;
    }
    auto touch = GenericAtomicAction(static_cast<ActionStage>(stage), Point(px, py));
    auto err = ApiCallErr(NO_ERROR);
    UiOpArgs uiOpArgs;
    driver.PerformTouch(touch, uiOpArgs, err);
    if (!ExtensionApiCheck(err.code_ == NO_ERROR)) {
        return RETCODE_FAIL;
    }
    return RETCODE_SUCCESS;
}

RetCode StopCapture(Text name)
{
    if (!ExtensionApiCheck(name.data != nullptr)) {
        return RETCODE_FAIL;
    }
    unique_lock<mutex> guard(g_captureLock);
    if (g_runningCaptures.find(name.data) == g_runningCaptures.end()) {
        return RETCODE_SUCCESS;
    }
    if (strcmp(name.data, "copyScreen") == 0) {
        StopScreenCopy();
    } else if (strcmp(name.data, "recordUiAction") == 0) {
        UiDriverRecordStop();
        unique_lock<mutex> lock(g_recordRunningLock);
    } else if (strcmp(name.data, "dumpLayout") != 0) {
        if (!ExtensionApiCheck(false)) {
            return RETCODE_FAIL;
        }
    }
    g_runningCaptures.erase(name.data);
    return RETCODE_SUCCESS;
}

RetCode GetDumpInfo(nlohmann::json &options, nlohmann::json &tree, ApiCallErr &err)
{
    static auto driver = MockDriver();
    DumpOption dumpOption;
    if (options.type() == nlohmann::detail::value_t::object && options.contains("bundleName")) {
        nlohmann::json val = options["bundleName"];
        if (!ExtensionApiCheck(val.type() == detail::value_t::string)) {
            return RETCODE_FAIL;
        }
        dumpOption.bundleName_ = val.get<string>();
    }
    if (options.type() == nlohmann::detail::value_t::object && options.contains("windowId")) {
        nlohmann::json val = options["windowId"];
        if (!ExtensionApiCheck(val.type() == detail::value_t::string)) {
            return RETCODE_FAIL;
        }
        auto windowId = val.get<string>();
        if (!ExtensionApiCheck(atoi(windowId.c_str()) != 0)) {
            return RETCODE_FAIL;
        }
        dumpOption.windowId_ = windowId;
    }
    if (options.type() == nlohmann::detail::value_t::object && options.contains("mergeWindow")) {
        nlohmann::json val = options["mergeWindow"];
        if (!ExtensionApiCheck(val.type() == detail::value_t::string)) {
            return RETCODE_FAIL;
        }
        auto mergeWindow = val.get<string>();
        if (mergeWindow == "false") {
            dumpOption.notMergeWindow_ = true;
        }
    }
    if (options.type() == nlohmann::detail::value_t::object && options.contains("displayId")) {
        nlohmann::json val = options["displayId"];
        if (!ExtensionApiCheck(val.type() == detail::value_t::number_integer)) {
            return RETCODE_FAIL;
        }
        dumpOption.displayId_ = val.get<int>();
    }
    driver.DumpUiHierarchy(tree, dumpOption, err);
    return RETCODE_SUCCESS;
}

RetCode StartCapture(Text name, DataCallback callback, Text optJson)
{
    static auto driver = MockDriver();
    if (!ExtensionApiCheck(name.data != nullptr && callback != nullptr)) {
        return RETCODE_FAIL;
    }
    nlohmann::json options = nullptr;
    if (optJson.data != nullptr) {
        options = nlohmann::json::parse(optJson.data, nullptr, false);
        if (!ExtensionApiCheck(!options.is_discarded())) {
            return RETCODE_FAIL;
        }
    }
    unique_lock<mutex> guard(g_captureLock);
    const auto running = g_runningCaptures.find(name.data) != g_runningCaptures.end();
    if (!ExtensionApiCheck(!running)) {
        return RETCODE_FAIL;
    }
    g_runningCaptures.insert(name.data);
    guard.unlock();
    if (strcmp(name.data, "dumpLayout") == 0) {
        nlohmann::json tree;
        ApiCallErr err(NO_ERROR);
        if (!ExtensionApiCheck(GetDumpInfo(options, tree, err) == RETCODE_SUCCESS)) {
            return RETCODE_FAIL;
        }
        g_runningCaptures.erase("dumpLayout"); // dumpLayout is sync&once
        if (!ExtensionApiCheck(err.code_ == NO_ERROR)) {
            return RETCODE_FAIL;
        }
        auto layout = tree.dump(-1, ' ', false, nlohmann::detail::error_handler_t::replace);
        callback(Text{layout.c_str(), layout.length()});
    } else if (strcmp(name.data, "copyScreen") == 0) {
        float scale = ReadArgFromJson<float>(options, "scale", 1.0f);
        int32_t displayId = ReadArgFromJson<int32_t>(options, "displayId", UNASSIGNED);
        StartScreenCopy(scale, displayId, [callback](uint8_t *data, size_t len) {
            callback(Text{reinterpret_cast<const char *>(data), len});
            free(data);
        });
    } else if (strcmp(name.data, "recordUiAction") == 0) {
        UiDriverRecordStop();
        unique_lock<mutex> lock(g_recordRunningLock);
        auto recordThread = thread([callback]() {
            unique_lock<mutex> lock(g_recordRunningLock);
        });
        recordThread.detach();
    } else {
        return RETCODE_FAIL;
    }
    return RETCODE_SUCCESS;
}
} // namespace OHOS::uitest
#endif
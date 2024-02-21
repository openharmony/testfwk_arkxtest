/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <cstdlib>
#include <string>
#include "extension_c_api.h"

using namespace std;
using namespace OHOS::uitest;

static uintptr_t g_onInitImplAddr = 0;
static uintptr_t g_onRunImplAddr = 0;

extern "C" RetCode UiTestExtension_OnInit(UiTestPort port, size_t argc, char **argv)
{
    constexpr int32_t INTERAL_ERROR_CODE = RETCODE_FAIL - 1;
    auto envOnInitImplAddr = getenv("OnInitImplAddr");
    auto envOnRunImplAddr = getenv("OnRunImplAddr");
    if (envOnInitImplAddr == nullptr || envOnRunImplAddr == nullptr) {
        return INTERAL_ERROR_CODE;
    }
    g_onInitImplAddr = std::stoul(envOnInitImplAddr);
    g_onRunImplAddr = std::stoul(envOnRunImplAddr);
    if (g_onInitImplAddr <= 0 || g_onRunImplAddr <= 0) {
        return INTERAL_ERROR_CODE;
    }
    auto impl = reinterpret_cast<UiTestExtensionOnInitCallback>(g_onInitImplAddr);
    return impl(port, argc, argv);
}

extern "C" RetCode UiTestExtension_OnRun()
{
    auto impl = reinterpret_cast<UiTestExtensionOnRunCallback>(g_onRunImplAddr);
    return impl();
}
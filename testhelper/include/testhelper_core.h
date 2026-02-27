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

#ifndef TESTHELPER_CORE_H
#define TESTHELPER_CORE_H

#include "common_utils.h"

namespace OHOS::testhelper {
    class TestHelperCore {
    public:
        int32_t HandleGetTime();
        int32_t HandleSetTime(const std::string& timeStr);
        int32_t HandleGetTimezone();
        int32_t HandleSetTimezone(const std::string& timezone);
        int32_t HandleGetPasteData();
        int32_t HandleSetPasteData(const std::string& text);
        int32_t HandleClearPasteData();
        int32_t HandleHideKeyboard();
 
    private:
        bool ParseTimeToMs(const std::string& timeStr, int64_t& timeMs);
    };
}

#endif // TESTHELPER_CORE_H

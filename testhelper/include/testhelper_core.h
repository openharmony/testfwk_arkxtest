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
#include "test_server_client.h"
#include "test_server_error_code.h"
#include <vector>

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
        int32_t HandleGetFontname(const std::string& fontPath);
        int32_t HandleInstallFont(const std::string& fontPath);
        int32_t HandleUninstallFont(const std::string& fontName);
        int32_t HandleSetViewMode(const std::string& mode);
        int32_t HandleEnableLocationMock();
        int32_t HandleDisableLocationMock();
        int32_t HandleSetMockedLocations(const std::string& gpxFile, int32_t timeInterval = 1);
        int32_t ParseTimeToMs(const std::string& timeStr, int64_t& timeMs);

    private:
        bool ValidateTimeRanges(int year, int month, int day, int hour, int minute, int second);
        bool ValidateFontFileFormat(const std::string& filePath);
        bool ValidateGPXFilePath(const std::string& filePath);
        bool ValidateTimeInterval(int32_t timeInterval);
    };
}

#endif // TESTHELPER_CORE_H

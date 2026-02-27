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

#include <iostream>
#include <string>
#include <ctime>
#include "common_utils.h"
#include "testhelper_core.h"
#include "time_service_client.h"
#include "test_server_client.h"
#include "test_server_error_code.h"

namespace OHOS::testhelper {
    int32_t TestHelperCore::HandleGetTime()
    {
        LOG_I("HandleGetTime called");
        auto timeService = MiscServices::TimeServiceClient::GetInstance();
        if (!timeService) {
            LOG_E("TimeService not available");
            PrintToConsole("Error: Service is not available.");
            return EXIT_SERVICE_UNAVAILABLE;
        }
        int64_t timeMs = 0;
        timeService->GetWallTimeMs(timeMs);
        time_t seconds = timeMs / MS_PER_SECOND;
        char buffer[TIME_BUFFER_SIZE];
        if (strftime(buffer, TIME_BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", localtime(&seconds)) == 0) {
            LOG_E("strftime failed");
            return EXIT_FAILURE;
        }
        PrintToConsole("Current system time: " + std::string(buffer));
        LOG_I("HandleGetTime completed successfully");
        return EXIT_SUCCESS;
    }

    int32_t TestHelperCore::HandleSetTime(const std::string& timeStr)
    {
        LOG_I("HandleSetTime called with time: %{public}s", timeStr.c_str());
        int64_t timeMs = 0;
        if (!ParseTimeToMs(timeStr, timeMs)) {
            LOG_E("Invalid time format: %{public}s", timeStr.c_str());
            PrintToConsole("Error: Invalid time format. Time must be in 'YYYY-MM-DD HH:MM:SS' format.");
            return EXIT_FAILURE;
        }
        auto result = OHOS::testserver::TestServerClient::GetInstance().SetTime(timeMs);
        if (result == OHOS::testserver::TEST_SERVER_OK) {
            PrintToConsole("Set time to " + timeStr + " successfully.");
            LOG_I("HandleSetTime completed successfully");
            return EXIT_SUCCESS;
        } else {
            LOG_E("SetTime failed with error: %{public}d", result);
            PrintToConsole("Error: Service is not available.");
            return EXIT_SERVICE_UNAVAILABLE;
        }
    }

    int32_t TestHelperCore::HandleGetTimezone()
    {
        LOG_I("HandleGetTimezone called");
        auto timeService = MiscServices::TimeServiceClient::GetInstance();
        if (!timeService) {
            LOG_E("TimeService not available");
            PrintToConsole("Error: Service is not available.");
            return EXIT_SERVICE_UNAVAILABLE;
        }
        std::string timezoneId;
        timeService->GetTimeZone(timezoneId);
        PrintToConsole("Current timezone: " + timezoneId);
        LOG_I("HandleGetTimezone completed successfully");
        return EXIT_SUCCESS;
    }

    int32_t TestHelperCore::HandleSetTimezone(const std::string& timezone)
    {
        LOG_I("HandleSetTimezone called with timezone: %{public}s", timezone.c_str());
        auto result = OHOS::testserver::TestServerClient::GetInstance().SetTimezone(timezone);
        if (result == OHOS::testserver::TEST_SERVER_OK) {
            PrintToConsole("Set timezone to " + timezone + " successfully.");
            LOG_I("HandleSetTimezone completed successfully");
            return EXIT_SUCCESS;
        } else if (result == OHOS::testserver::TEST_SERVER_INVALID_TIMEZONE_ID) {
            LOG_E("SetTimezone failed: invalid timezone value");
            PrintToConsole("Error: Invalid timezone value. " + timezone + " is not a valid timezone identifier.");
            return EXIT_FAILURE;
        } else {
            LOG_E("SetTimezone failed with error: %{public}d", result);
            PrintToConsole("Error: Service is not available.");
            return EXIT_SERVICE_UNAVAILABLE;
        }
    }

    bool TestHelperCore::ParseTimeToMs(const std::string& timeStr, int64_t& timeMs)
    {
        struct tm timeinfo = {};
        char* result = strptime(timeStr.c_str(), "%Y-%m-%d %H:%M:%S", &timeinfo);
        if (result == nullptr || *result != '\0') {
            return false;
        }
        int year = timeinfo.tm_year + BASE_YEAR;
        int month = timeinfo.tm_mon + BASE_MONTH;
        int day = timeinfo.tm_mday;
        int hour = timeinfo.tm_hour;
        int minute = timeinfo.tm_min;
        int second = timeinfo.tm_sec;
        if (year < MIN_YEAR || month < MIN_MONTH || month > MAX_MONTH || day < MIN_DAY || day > MAX_DAY ||
            hour < MIN_HOUR || hour > MAX_HOUR || minute < MIN_MINUTE || minute > MAX_MINUTE || second < MIN_SECOND ||
            second > MAX_SECOND) {
            return false;
        }
        timeinfo.tm_isdst = -1;
        time_t seconds = mktime(&timeinfo);
        if (seconds == -1) {
            return false;
        }
        timeMs = static_cast<int64_t>(seconds) * MS_PER_SECOND;
        if (timeMs / MS_PER_SECOND > MAX_TIMESTAMP_SECONDS) {
            return false;
        }
        return true;
    }

    int32_t TestHelperCore::HandleGetPasteData()
    {
        LOG_I("HandleGetPasteData called");
        std::string pasteText;
        auto result = OHOS::testserver::TestServerClient::GetInstance().GetPasteData(pasteText);
        if (result == OHOS::testserver::TEST_SERVER_OK) {
            PrintToConsole("Pasteboard text: " + pasteText);
            LOG_I("HandleGetPasteData completed successfully");
            return EXIT_SUCCESS;
        } else if (result == OHOS::testserver::TEST_SERVER_NOT_SUPPORTED) {
            LOG_E("Pasteboard feature is not supported");
            PrintToConsole("Error: Operation is not supported. Pasteboard is not supported on this device.");
            return EXIT_FAILURE;
        } else {
            LOG_E("GetPasteData failed with error: %{public}d", result);
            PrintToConsole("Error: Service is not available.");
            return EXIT_SERVICE_UNAVAILABLE;
        }
    }

    int32_t TestHelperCore::HandleSetPasteData(const std::string& text)
    {
        LOG_I("HandleSetPasteData called with text length: %{public}zu", text.length());
        
        const size_t MAX_TEXT_SIZE = 128 * 1024 * 1024;
        if (text.length() > MAX_TEXT_SIZE) {
            LOG_E("Text too long: %{public}zu bytes", text.length());
            PrintToConsole("Error: Text is too long. Maximum length is 128MB.");
            return EXIT_FAILURE;
        }
        
        auto result = OHOS::testserver::TestServerClient::GetInstance().SetPasteData(text);
        if (result == OHOS::testserver::TEST_SERVER_OK) {
            PrintToConsole("Pasteboard text set successfully.");
            LOG_I("HandleSetPasteData completed successfully");
            return EXIT_SUCCESS;
        } else if (result == OHOS::testserver::TEST_SERVER_NOT_SUPPORTED) {
            LOG_E("Pasteboard feature is not supported");
            PrintToConsole("Error: Operation is not supported. Pasteboard is not supported on this device.");
            return EXIT_FAILURE;
        } else {
            LOG_E("SetPasteData failed with error: %{public}d", result);
            PrintToConsole("Error: Service is not available.");
            return EXIT_SERVICE_UNAVAILABLE;
        }
    }

    int32_t TestHelperCore::HandleClearPasteData()
    {
        LOG_I("HandleClearPasteData called");
        auto result = OHOS::testserver::TestServerClient::GetInstance().ClearPasteData();
        if (result == OHOS::testserver::TEST_SERVER_OK) {
            PrintToConsole("Pasteboard text cleared successfully.");
            LOG_I("HandleClearPasteData completed successfully");
            return EXIT_SUCCESS;
        } else if (result == OHOS::testserver::TEST_SERVER_NOT_SUPPORTED) {
            LOG_E("Pasteboard feature is not supported");
            PrintToConsole("Error: Operation is not supported. Pasteboard is not supported on this device.");
            return EXIT_FAILURE;
        } else {
            LOG_E("ClearPasteData failed with error: %{public}d", result);
            PrintToConsole("Error: Service is not available.");
            return EXIT_SERVICE_UNAVAILABLE;
        }
    }

    int32_t TestHelperCore::HandleHideKeyboard()
    {
        LOG_I("HandleHideKeyboard called");
        auto result = OHOS::testserver::TestServerClient::GetInstance().HideKeyboard();
        if (result == OHOS::testserver::TEST_SERVER_OK) {
            PrintToConsole("Keyboard hidden successfully.");
            LOG_I("HandleHideKeyboard completed successfully");
            return EXIT_SUCCESS;
        } else if (result == OHOS::testserver::TEST_SERVER_NOT_SUPPORTED) {
            LOG_E("IMF feature is not supported");
            PrintToConsole("Error: Operation is not supported. IMF is not supported on this device.");
            return EXIT_FAILURE;
        } else if (result == OHOS::testserver::TEST_SERVER_NO_ACTIVE_IME) {
            LOG_E("No active IME client");
            PrintToConsole("Error: No active input method keyboard.");
            return EXIT_FAILURE;
        } else {
            LOG_E("HideKeyboard failed with error: %{public}d", result);
            PrintToConsole("Error: Service is not available.");
            return EXIT_SERVICE_UNAVAILABLE;
        }
    }
}
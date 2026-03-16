/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "testhelper_core.h"
#include "common_utils.h"
using namespace OHOS::testhelper;

class ParseTimeToMsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ParseTimeToMsTest, ValidTime_Normal)
{
    std::string timeStr = "2026-02-28 14:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
    EXPECT_GT(timeMs, 0);
}

TEST_F(ParseTimeToMsTest, ValidTime_Midnight)
{
    std::string timeStr = "2026-02-28 00:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
    EXPECT_GT(timeMs, 0);
}

TEST_F(ParseTimeToMsTest, ValidTime_EndOfDay)
{
    std::string timeStr = "2026-02-28 23:59:59";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
    EXPECT_GT(timeMs, 0);
}

TEST_F(ParseTimeToMsTest, Valid_Date_Feb29_LeapYear)
{
    std::string timeStr = "2024-02-29 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Format_CaseInsensitive)
{
    std::string timeStr = "2026-02-28 14:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Format_WithLeadingZeros)
{
    std::vector<std::string> validFormats = {
        "2026-02-28 14:30:00",
        "2026-12-01 23:59:59",
        "2026-01-01 00:00:00"
    };
    for (const auto& timeStr : validFormats) {
        int64_t timeMs = 0;
        int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
        EXPECT_EQ(result, PARSE_TIME_SUCCESS) << "Failed for: " << timeStr;
    }
}

TEST_F(ParseTimeToMsTest, InvalidFormat_SingleDigitMonth)
{
    std::string timeStr = "2026-1-28 14:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidFormat_SingleDigitDay)
{
    std::string timeStr = "2026-02-8 14:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidFormat_SingleDigitHour)
{
    std::string timeStr = "2026-02-28 4:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidFormat_SingleDigitMinute)
{
    std::string timeStr = "2026-02-28 14:5:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidFormat_SingleDigitSecond)
{
    std::string timeStr = "2026-02-28 14:30:5";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidFormat_SlashSeparator)
{
    std::string timeStr = "2026/02/28 14:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidFormat_MissingSeconds)
{
    std::string timeStr = "2026-02-28 14:30";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidFormat_EmptyString)
{
    std::string timeStr = "";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidFormat_ShortString)
{
    std::vector<std::string> shortFormats = {
        "2026-02-28 14:30",
        "2026-02-28",
        "2026-02",
        "2026"
    };
    for (const auto& timeStr : shortFormats) {
        int64_t timeMs = 0;
        int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
        EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT) << "Failed for: " << timeStr;
    }
}

TEST_F(ParseTimeToMsTest, InvalidFormat_SpecialCharacters)
{
    std::vector<std::string> invalidFormats = {
        "2026-02-28 14:30:00\n",
        "2026-02-28 14:30:00\r",
        "2026-02-28 14:30:00\t",
        "2026-02-28 14:30:00 ",
        " 2026-02-28 14:30:00",
        "2026-02-28 14:30:00.",
        "2026-02-28 14:30:00,"
    };
    for (const auto& timeStr : invalidFormats) {
        int64_t timeMs = 0;
        int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
        EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT) << "Failed for: " << timeStr;
    }
}

TEST_F(ParseTimeToMsTest, InvalidFormat_MixedSeparators)
{
    std::vector<std::string> mixedFormats = {
        "2026/02-28 14:30:00",
        "2026-02/28 14:30:00",
        "2026-02-28 14.30:00",
        "2026-02-28 14:30;00",
        "2026:02:28 14:30:00"
    };
    for (const auto& timeStr : mixedFormats) {
        int64_t timeMs = 0;
        int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
        EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT) << "Failed for: " << timeStr;
    }
}

TEST_F(ParseTimeToMsTest, Invalid_Time_NegativeHour)
{
    std::string timeStr = "2026-01-01 -01:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_FORMAT);
}

TEST_F(ParseTimeToMsTest, InvalidDate_Feb30)
{
    std::string timeStr = "2026-02-30 14:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, InvalidDate_Apr31)
{
    std::string timeStr = "2026-04-31 14:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Invalid_Date_Feb29_NonLeapYear)
{
    std::string timeStr = "2023-02-29 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, InvalidTime_Hour24)
{
    std::string timeStr = "2026-02-28 24:30:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, InvalidTime_Minute60)
{
    std::string timeStr = "2026-02-28 14:60:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, InvalidTime_Second60)
{
    std::string timeStr = "2026-02-28 14:30:60";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_BelowMinYear)
{
    std::string timeStr = "1969-12-31 23:59:59";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_Year2038)
{
    TestHelperCore().HandleSetTimezone("Europe/London");
    std::string timeStr = "2038-01-19 03:14:07";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_MaxYear)
{
    std::string timeStr = "2037-12-31 23:59:59";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_MonthMin)
{
    std::string timeStr = "2026-01-01 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_MonthMax)
{
    std::string timeStr = "2026-12-31 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_MonthInvalid)
{
    std::string timeStr = "2026-00-01 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_MonthInvalid13)
{
    std::string timeStr = "2026-13-01 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_DayInvalid0)
{
    std::string timeStr = "2026-01-00 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_DayInvalid32)
{
    std::string timeStr = "2026-01-32 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_HourMin)
{
    std::string timeStr = "2026-01-01 00:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_HourMax)
{
    std::string timeStr = "2026-01-01 23:59:59";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_HourInvalid)
{
    std::string timeStr = "2026-01-01 25:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_MinuteMin)
{
    std::string timeStr = "2026-01-01 00:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_MinuteMax)
{
    std::string timeStr = "2026-01-01 00:59:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_MinuteInvalid)
{
    std::string timeStr = "2026-01-01 00:60:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_SecondMin)
{
    std::string timeStr = "2026-01-01 00:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_SecondMax)
{
    std::string timeStr = "2026-01-01 00:00:59";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Boundary_SecondInvalid)
{
    std::string timeStr = "2026-01-01 00:00:60";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_UtcMaxTimestamp)
{
    TestHelperCore().HandleSetTimezone("Europe/London");
    std::string timeStr = "2038-01-19 03:14:07";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
    EXPECT_EQ(timeMs / MS_PER_SECOND, MAX_TIMESTAMP_SECONDS);
}

TEST_F(ParseTimeToMsTest, Boundary_AsiaShanghaiMaxTimestamp)
{
    TestHelperCore().HandleSetTimezone("Asia/Shanghai");
    std::string timeStr = "2038-01-19 11:14:07";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
    EXPECT_EQ(timeMs / MS_PER_SECOND, MAX_TIMESTAMP_SECONDS);
}

TEST_F(ParseTimeToMsTest, Boundary_UtcMaxTimestampPlusOneSecond)
{
    TestHelperCore().HandleSetTimezone("Europe/London");
    std::string timeStr = "2038-01-19 03:14:08";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Boundary_AsiaShanghaiMaxTimestampPlusOneSecond)
{
    TestHelperCore().HandleSetTimezone("Asia/Shanghai");
    std::string timeStr = "2038-01-19 11:14:08";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_INVALID_VALUE);
}

TEST_F(ParseTimeToMsTest, Special_Date_FirstDayOfYear)
{
    std::string timeStr = "2026-01-01 00:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Special_Date_LastDayOfYear)
{
    std::string timeStr = "2026-12-31 23:59:59";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Special_Date_LeapYearDivisibleBy4)
{
    std::string timeStr = "2024-02-29 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

TEST_F(ParseTimeToMsTest, Special_Date_LeapYearNotDivisibleBy100)
{
    std::string timeStr = "2000-02-29 12:00:00";
    int64_t timeMs = 0;
    int32_t result = TestHelperCore().ParseTimeToMs(timeStr, timeMs);
    EXPECT_EQ(result, PARSE_TIME_SUCCESS);
}

class TimeOperationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TimeOperationTest, HandleGetTime_Success)
{
    int32_t result = TestHelperCore().HandleGetTime();
    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(TimeOperationTest, HandleGetTimezone_Success)
{
    int32_t result = TestHelperCore().HandleGetTimezone();
    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(TimeOperationTest, HandleSetTime_ValidTime)
{
    std::string validTime = "2026-02-28 14:30:00";
    int32_t result = TestHelperCore().HandleSetTime(validTime);
    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(TimeOperationTest, HandleSetTime_MaxTimestamp_Utc)
{
    TestHelperCore().HandleSetTimezone("Europe/London");
    std::string maxTime = "2038-01-19 03:14:07";
    int32_t result = TestHelperCore().HandleSetTime(maxTime);
    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(TimeOperationTest, HandleSetTime_MaxTimestamp_AsiaShanghai)
{
    TestHelperCore().HandleSetTimezone("Asia/Shanghai");
    std::string maxTime = "2038-01-19 11:14:07";
    int32_t result = TestHelperCore().HandleSetTime(maxTime);
    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(TimeOperationTest, HandleSetTime_InvalidFormat)
{
    std::vector<std::string> invalidTimes = {
        "2026-02-28 14:30",
        "2026/02/28 14:30:00",
        "2026-02-28 4:30:00",
        "2026-02-8 14:30:00"
    };
    for (const auto& timeStr : invalidTimes) {
        int32_t result = TestHelperCore().HandleSetTime(timeStr);
        EXPECT_EQ(result, EXIT_FAILURE);
    }
}

TEST_F(TimeOperationTest, HandleSetTime_InvalidDate)
{
    std::vector<std::string> invalidDates = {
        "2026-02-30 14:30:00",
        "2026-04-31 14:30:00",
        "2026-01-32 14:30:00",
        "2023-02-29 14:30:00"
    };
    for (const auto& dateStr : invalidDates) {
        int32_t result = TestHelperCore().HandleSetTime(dateStr);
        EXPECT_EQ(result, EXIT_FAILURE);
    }
}

TEST_F(TimeOperationTest, HandleSetTime_MaxTimestamp_UtcPlusOneSecond)
{
    TestHelperCore().HandleSetTimezone("Europe/London");
    std::string invalidTime = "2038-01-19 03:14:08";
    int32_t result = TestHelperCore().HandleSetTime(invalidTime);
    EXPECT_EQ(result, EXIT_FAILURE);
}

TEST_F(TimeOperationTest, HandleSetTime_MaxTimestamp_AsiaShanghaiPlusOneSecond)
{
    TestHelperCore().HandleSetTimezone("Asia/Shanghai");
    std::string invalidTime = "2038-01-19 11:14:08";
    int32_t result = TestHelperCore().HandleSetTime(invalidTime);
    EXPECT_EQ(result, EXIT_FAILURE);
}

TEST_F(TimeOperationTest, HandleSetTimezone_BoundaryTimezones)
{
    std::vector<std::string> boundaryTimezones = {
        "Asia/Shanghai",
        "America/New_York",
        "Europe/London",
        "Asia/Tokyo",
        "Pacific/Kiritimati",
        "Pacific/Enderbury",
        "America/Metlakatla",
        "Etc/GMT+12",
        "Etc/GMT-12"
    };
    for (const auto& timezone : boundaryTimezones) {
        int32_t result = TestHelperCore().HandleSetTimezone(timezone);
        EXPECT_EQ(result, EXIT_SUCCESS);
    }
}

TEST_F(TimeOperationTest, HandleSetTimezone_InvalidTimezone)
{
    std::vector<std::string> invalidTimezones = {
        "",
        "Invalid/Zone",
        "UTC-12:00",
        "Asia/InvalidCity",
        "NotATimezone",
        "Pacific/Invalid"
    };
    for (const auto& timezone : invalidTimezones) {
        int32_t result = TestHelperCore().HandleSetTimezone(timezone);
        EXPECT_EQ(result, EXIT_FAILURE);
    }
}

class ClipboardOperationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    static constexpr int32_t bytesPerKb = 1024;
    static constexpr int32_t maxClipboardSizeMb = 128;
};

TEST_F(ClipboardOperationTest, HandleSetPasteData_ValidText)
{
    std::vector<std::string> validTexts = {
        "Hello World",
        "Test text with numbers 123",
        "Special characters !@#$%",
        "",
        "A"
    };
    for (const auto& text : validTexts) {
        int32_t result = TestHelperCore().HandleSetPasteData(text);
        EXPECT_EQ(result, EXIT_SUCCESS);
    }
}

TEST_F(ClipboardOperationTest, HandleSetPasteData_LargeText)
{
    std::string largeText(bytesPerKb * bytesPerKb, 'A');
    int32_t result = TestHelperCore().HandleSetPasteData(largeText);
    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(ClipboardOperationTest, HandleGetPasteData_Success)
{
    int32_t result = TestHelperCore().HandleGetPasteData();
    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(ClipboardOperationTest, HandleClearPasteData_Success)
{
    int32_t result = TestHelperCore().HandleClearPasteData();
    EXPECT_EQ(result, EXIT_SUCCESS);
}

TEST_F(ClipboardOperationTest, HandleSetPasteData_OversizedText)
{
    std::string oversizedText(maxClipboardSizeMb * bytesPerKb * bytesPerKb + 1, 'B');
    int32_t result = TestHelperCore().HandleSetPasteData(oversizedText);
    EXPECT_EQ(result, EXIT_FAILURE);
}

class KeyboardOperationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(KeyboardOperationTest, HandleHideKeyboard)
{
    int32_t result = TestHelperCore().HandleHideKeyboard();
    EXPECT_EQ(result, EXIT_FAILURE);
}

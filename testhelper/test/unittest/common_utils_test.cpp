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

#include <gtest/gtest.h>
#include <string>
#include "common_utils.h"

using namespace OHOS::testhelper;

class CommonUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CommonUtilsTest, GenLogTag_SimpleCase)
{
    std::string_view fileName = "test.cpp";
    std::string_view funcName = "TestFunc";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[test.cpp:(TestFunc)]");
}

TEST_F(CommonUtilsTest, GenLogTag_WithPath)
{
    std::string_view fileName = "path/to/test.cpp";
    std::string_view funcName = "TestFunc";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[test.cpp:(TestFunc)]");
}

TEST_F(CommonUtilsTest, GenLogTag_SpecialCharsInFilename)
{
    std::string_view fileName = "test-file.cpp";
    std::string_view funcName = "TestFunc";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[test-file.cpp:(TestFunc)]");
}

TEST_F(CommonUtilsTest, GenLogTag_SpecialCharsInFuncname)
{
    std::string_view fileName = "test.cpp";
    std::string_view funcName = "Test_Func_123";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[test.cpp:(Test_Func_123)]");
}

TEST_F(CommonUtilsTest, GenLogTag_SpecialCharsInBoth)
{
    std::string_view fileName = "path/with-dashes/and_underscores/file.cpp";
    std::string_view funcName = "Func_Name";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[file.cpp:(Func_Name)]");
}

TEST_F(CommonUtilsTest, GenLogTag_TemplateFunction)
{
    std::string_view fileName = "test.cpp";
    std::string_view funcName = "Test::Function_With::Template<int>";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[test.cpp:(Test::Function_With::Template<int>)]");
}

TEST_F(CommonUtilsTest, GenLogTag_LongPath_NormalFunc)
{
    std::string_view fileName = "very/long/path/to/file/test.cpp";
    std::string_view funcName = "ShortFunc";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[test.cpp:(ShortFunc)]");
}

TEST_F(CommonUtilsTest, GenLogTag_LongFilename_TruncatedFunc)
{
    std::string_view longFile = "very_long_filename_that_will_be_truncated_test.cpp";
    std::string_view longFunc = "VeryLongFunctionNameThatExceedsTheLimit";
    auto tagChars = GenLogTag(longFile, longFunc);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[very_long_filename_that_will_be_truncated_test.cpp:(VeryLongFu");
}

TEST_F(CommonUtilsTest, GenLogTag_VeryLongFilename_ShortFunc)
{
    std::string_view longFile = "src/modules/very_long_module_name/impl/very_long_file_name.cpp";
    std::string_view funcName = "F";
    auto tagChars = GenLogTag(longFile, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[very_long_file_name.cpp:(F)]");
}

TEST_F(CommonUtilsTest, GenLogTag_ShortFilename_VeryLongFunc)
{
    std::string_view fileName = "t.cpp";
    std::string_view longFunc = "VeryLongFunctionNameThatMightExceedTheMaximumAllowedLength";
    auto tagChars = GenLogTag(fileName, longFunc);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[t.cpp:(VeryLongFunctionNameThatMightExceedTheMaximumAllowedLen");
}

TEST_F(CommonUtilsTest, GenLogTag_BothVeryLong_MaxLength)
{
    std::string_view longFile =
        "extremely_long_filename_that_will_definitely_exceed_the_maximum_allowed_length_test.cpp";
    std::string_view longFunc = "VeryLongFunctionNameThatWillAlsoExceedTheLimitCombined";
    auto tagChars = GenLogTag(longFile, longFunc);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tagLen, MAX_LOG_TAG_LEN - 1);
    EXPECT_EQ(tag, "[extremely_long_filename_that_will_definitely_exceed_the_maximu");
}

TEST_F(CommonUtilsTest, GenLogTag_NoPathSeparator)
{
    std::string_view fileName = "simple.cpp";
    std::string_view funcName = "SimpleFunc";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[simple.cpp:(SimpleFunc)]");
}

TEST_F(CommonUtilsTest, GenLogTag_EmptyFilename)
{
    std::string_view fileName = "";
    std::string_view funcName = "TestFunc";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[:(TestFunc)]");
}

TEST_F(CommonUtilsTest, GenLogTag_EmptyFuncname)
{
    std::string_view fileName = "test.cpp";
    std::string_view funcName = "";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[test.cpp:()]");
}

TEST_F(CommonUtilsTest, GenLogTag_BothEmpty)
{
    std::string_view fileName = "";
    std::string_view funcName = "";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[:()]");
}

TEST_F(CommonUtilsTest, GenLogTag_TruncateBeforeColon)
{
    std::string_view longFile =
        "a_very_long_filename_that_exceeds_the_maximum_allowed_length_and_will_force_truncation_test.cpp";
    std::string_view funcName = "Func";
    auto tagChars = GenLogTag(longFile, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tagLen, MAX_LOG_TAG_LEN - 1);
    EXPECT_EQ(tag, "[a_very_long_filename_that_exceeds_the_maximum_allowed_length_a");
}

TEST_F(CommonUtilsTest, GenLogTag_TruncateBeforeParen)
{
    std::string_view fileName = "very_long_filename_without_separator_test.cpp";
    std::string_view funcName = "";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[very_long_filename_without_separator_test.cpp:()]");
}

TEST_F(CommonUtilsTest, GenLogTag_MaxExactLength)
{
    std::string_view fileName = "exact_55_chars_filename_to_reach_max_tag_length_test.cpp";
    std::string_view funcName = "A";
    auto tagChars = GenLogTag(fileName, funcName);
    int8_t tagLen = tagChars[MAX_LOG_TAG_LEN - 1];
    auto tag = std::string_view(tagChars.data(), tagLen);
    EXPECT_EQ(tag, "[exact_55_chars_filename_to_reach_max_tag_length_test.cpp:(A)]");
}

class PrintToConsoleTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    static constexpr size_t longMessageSize = 10000;
};

TEST_F(PrintToConsoleTest, PrintToConsole_EmptyMessage)
{
    EXPECT_NO_THROW({
        PrintToConsole("");
    });
}

TEST_F(PrintToConsoleTest, PrintToConsole_SimpleMessage)
{
    EXPECT_NO_THROW({
        PrintToConsole("Hello, World!");
    });
}

TEST_F(PrintToConsoleTest, PrintToConsole_SpecialCharacters)
{
    EXPECT_NO_THROW({
        PrintToConsole("Special chars: !@#$%^&*()_+-=[]{}|;':\",./<>?");
    });
}

TEST_F(PrintToConsoleTest, PrintToConsole_UncodeCharacters)
{
    EXPECT_NO_THROW({
        PrintToConsole("Unicode: 中文测试 🎉 Ñoño");
    });
}

TEST_F(PrintToConsoleTest, PrintToConsole_Newline)
{
    EXPECT_NO_THROW({
        PrintToConsole("Line 1\nLine 2\nLine 3");
    });
}

TEST_F(PrintToConsoleTest, PrintToConsole_LongMessage)
{
    std::string longMessage(longMessageSize, 'A');
    EXPECT_NO_THROW({
        PrintToConsole(longMessage);
    });
}

TEST_F(PrintToConsoleTest, PrintToConsole_WithTabs)
{
    EXPECT_NO_THROW({
        PrintToConsole("Tab\tspace\ttest");
    });
}

TEST_F(PrintToConsoleTest, PrintToConsole_NumericMessage)
{
    EXPECT_NO_THROW({
        PrintToConsole("1234567890");
    });
}

TEST_F(PrintToConsoleTest, PrintToConsole_MixedContent)
{
    EXPECT_NO_THROW({
        PrintToConsole("Error: Code 42 occurred at line 123");
    });
}

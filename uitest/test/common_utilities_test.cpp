/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "common_utilities_hpp.h"

using namespace OHOS::uitest;
using namespace std;

TEST(UtilitiesTest, testLogTagGenerator)
{
    // 'GenTag' should be ensured to work at compile time
    constexpr string_view fileName = "/abc/def/ghi.cpp";
    constexpr string_view function = "function";
    constexpr array<char, MAX_LOG_TAG_LEN> tagChars = GenLogTag(fileName, function);
    // tag len is stored at the last char
    constexpr size_t actualTagLen = tagChars.at(MAX_LOG_TAG_LEN - 1);
    constexpr string_view expectedTag = "[ghi.cpp:(function)]";
    static_assert(actualTagLen == expectedTag.length());
    const string_view actualTag = string_view(tagChars.data(), actualTagLen);
    ASSERT_TRUE(actualTag.compare(expectedTag) == 0);
}

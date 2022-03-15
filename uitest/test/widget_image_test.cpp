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

#include "widget_image.h"
#include "ui_model.h"
#include "gtest/gtest.h"

using namespace OHOS::uitest;
using namespace std;

static constexpr auto ATTR_TEXT = "text";

TEST(WidgetImageTest, compareEquals)
{
    map<string, string> attributes;
    attributes[ATTR_HASHCODE] = "123";
    attributes[ATTR_TEXT] = "wyz";
    attributes["id"] = "888";
    WidgetImage image0;
    image0.SetAttributes(attributes);

    WidgetImage image1;
    image1.SetAttributes(attributes);
    // all attributes are same, should be equal
    ASSERT_TRUE(image0.Compare(image1));

    attributes[ATTR_TEXT] = "wlj";
    image1.SetAttributes(attributes);
    // the hashcode are equal, difference of other attributes should be ignored
    ASSERT_TRUE(image0.Compare(image1));

    attributes[ATTR_HASHCODE] = "321";
    attributes[ATTR_TEXT] = "wyz";
    image1.SetAttributes(attributes);
    // the hashcode are not equal, all the other attributes must be same to make equal
    ASSERT_TRUE(image0.Compare(image1));
    attributes[ATTR_HASHCODE] = "321";
    attributes[ATTR_TEXT] = "zl";
    image1.SetAttributes(attributes);
    ASSERT_FALSE(image0.Compare(image1));
}

TEST(WidgetImageTest, setGetAttributes)
{
    map<string, string> attributes;
    attributes[ATTR_HASHCODE] = "123";
    attributes[ATTR_TEXT] = "wyz";
    attributes["id"] = "888";
    WidgetImage image0;
    image0.SetAttributes(attributes);

    // should retrieve correct and full attributes
    for (auto&[key, val]:attributes) {
        ASSERT_EQ(val, image0.GetAttribute(key, ""));
    }
}
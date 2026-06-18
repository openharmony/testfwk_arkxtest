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
#include "../core/ui_model.h"
#include <regex.h>
#include "nlohmann/json.hpp"

using namespace OHOS::uitest;
using namespace std;

static constexpr auto ATTR_TEXT = "text";
static constexpr auto ATTR_ID = "id";

TEST(REGEXPTestTrue, testRegex)
{
    // make a widget object
    Widget widget("hierarchy");
    widget.SetAttr(UiAttr::TEXT, "checkBox1");
    widget.SetAttr(UiAttr::ID, "btnTest");
    widget.SetAttr(UiAttr::HINT, "btnTest12345");
    widget.SetBounds(Rect(1, 2, 3, 4));
    // use regex to match widget
    auto matcherTxt = WidgetMatchModel(UiAttr::TEXT, "checkBox\\w", ValueMatchPattern::REG_EXP);
    auto matcherId = WidgetMatchModel(UiAttr::ID, "btn\\w{2,4}", ValueMatchPattern::REG_EXP);
    bool matchResultId = widget.MatchAttr(matcherId);
    bool matchResultTxt = widget.MatchAttr(matcherTxt);
    ASSERT_EQ(true, matchResultId);
    ASSERT_EQ(true, matchResultTxt);
    
    // use regex to match widget, use ValueMatchPattern::REG_EXP_ICASE
    auto matcherTxtIcase = WidgetMatchModel(UiAttr::TEXT, "^Check\\w{2,5}$", ValueMatchPattern::REG_EXP_ICASE);
    bool matchResultTxtIcase = widget.MatchAttr(matcherTxtIcase);
    ASSERT_EQ(true, matchResultTxtIcase);
}

TEST(REGEXPTestFalse, testRegex)
{
    // make a widget object
    Widget widget("hierarchy");
    widget.SetAttr(UiAttr::TEXT, "checkBox1");
    widget.SetAttr(UiAttr::ID, "btnTest");
    widget.SetAttr(UiAttr::HINT, "btnTest12345");
    widget.SetBounds(Rect(1, 2, 3, 4));
    // use regex to match widget
    auto matcherTxt1 = WidgetMatchModel(UiAttr::TEXT, "^Check\\w{2,5}$", ValueMatchPattern::REG_EXP);
    bool matchResultTxt1 = widget.MatchAttr(matcherTxt1);
    ASSERT_EQ(false, matchResultTxt1);

    auto matcherTxt2 = WidgetMatchModel(UiAttr::HINT, "^btn_Test\\w{2,3}$", ValueMatchPattern::REG_EXP_ICASE);
    bool matchResultTxt2 = widget.MatchAttr(matcherTxt2);
    ASSERT_EQ(false, matchResultTxt2);
}


TEST(RectTest, testRectBase)
{
    Rect rect(100, 200, 300, 400);
    ASSERT_EQ(100, rect.left_);
    ASSERT_EQ(200, rect.right_);
    ASSERT_EQ(300, rect.top_);
    ASSERT_EQ(400, rect.bottom_);

    ASSERT_EQ(rect.GetCenterX(), (100 + 200) / 2);
    ASSERT_EQ(rect.GetCenterY(), (300 + 400) / 2);
    ASSERT_EQ(rect.GetHeight(), 400 - 300);
    ASSERT_EQ(rect.GetWidth(), 200 - 100);
}

TEST(WidgetTest, testAttributes)
{
    Widget widget("hierarchy");
    // get not-exist attribute, should return default value
    ASSERT_EQ("none", widget.GetAttr(UiAttr::MAX));
    // get exist attribute, should return actual value
    widget.SetAttr(UiAttr::MAX, "wyz");
    ASSERT_EQ("", widget.GetAttr(UiAttr::TEXT));
}

/** NOTE:: Widget need to be movable since we need to move a constructed widget to
 * its hosting tree. We must ensure that the move-created widget be same as the
 * moved one (No any attribute/filed should be lost during moving).
 * */
TEST(WidgetTest, testSafeMovable)
{
    Widget widget("hierarchy");
    widget.SetAttr(UiAttr::TEXT, "wyz");
    widget.SetAttr(UiAttr::ID, "100");
    widget.SetBounds(Rect(1, 2, 3, 4));

    auto newWidget = move(widget);
    ASSERT_EQ("hierarchy", newWidget.GetHierarchy());
    ASSERT_EQ("wyz", newWidget.GetAttr(UiAttr::TEXT));
    ASSERT_EQ("100", newWidget.GetAttr(UiAttr::ID));
    auto bounds = newWidget.GetBounds();
    ASSERT_EQ(1, bounds.left_);
    ASSERT_EQ(2, bounds.right_);
    ASSERT_EQ(3, bounds.top_);
    ASSERT_EQ(4, bounds.bottom_);
}

TEST(WidgetTest, testToStr)
{
    Widget widget("hierarchy");

    // get exist attribute, should return default value
    widget.SetAttr(UiAttr::TEXT, "wyz");
    widget.SetAttr(UiAttr::ID, "100");

    auto str0 = widget.ToStr();
    ASSERT_TRUE(str0.find(ATTR_TEXT) != string::npos);
    ASSERT_TRUE(str0.find("wyz") != string::npos);
    ASSERT_TRUE(str0.find(ATTR_ID) != string::npos);
    ASSERT_TRUE(str0.find("100") != string::npos);

    // change attribute
    widget.SetAttr(UiAttr::ID, "211");
    str0 = widget.ToStr();
    ASSERT_TRUE(str0.find(ATTR_TEXT) != string::npos);
    ASSERT_TRUE(str0.find("wyz") != string::npos);
    ASSERT_TRUE(str0.find(ATTR_ID) != string::npos);
    ASSERT_TRUE(str0.find("211") != string::npos);
    ASSERT_TRUE(str0.find("100") == string::npos);
}

TEST(WidgetTest, testClone)
{
    Widget widget("hierarchy");
    // get exist attribute, should return default value
    widget.SetAttr(UiAttr::TEXT, "Camera");
    widget.SetAttr(UiAttr::ACCESSIBILITY_ID, "12");
    auto ret = widget.Clone("TEST");
    ASSERT_TRUE(ret->GetAttr(UiAttr::TEXT) == "Camera");
}

TEST(DumpHandlerTest, DumpWindowInfoToJson_FieldsNotChanged)
{
    std::vector<Widget> allWidget;
    Widget rootWidget("ROOT");
    rootWidget.SetAttr(UiAttr::ACCESSIBILITY_ID, "root_accessibility");
    rootWidget.SetAttr(UiAttr::ID, "root_id");
    rootWidget.SetAttr(UiAttr::TEXT, "RootWidget");
    rootWidget.SetAttr(UiAttr::KEY, "root_key");
    rootWidget.SetAttr(UiAttr::DESCRIPTION, "Root Description");
    rootWidget.SetAttr(UiAttr::TYPE, "Window");
    rootWidget.SetAttr(UiAttr::ENABLED, "true");
    rootWidget.SetAttr(UiAttr::FOCUSED, "false");
    rootWidget.SetAttr(UiAttr::SELECTED, "false");
    rootWidget.SetAttr(UiAttr::CLICKABLE, "true");
    rootWidget.SetAttr(UiAttr::LONG_CLICKABLE, "false");
    rootWidget.SetAttr(UiAttr::SCROLLABLE, "false");
    rootWidget.SetAttr(UiAttr::CHECKABLE, "false");
    rootWidget.SetAttr(UiAttr::CHECKED, "false");
    rootWidget.SetAttr(UiAttr::HOST_WINDOW_ID, "100");
    rootWidget.SetAttr(UiAttr::DISPLAY_ID, "0");
    rootWidget.SetAttr(UiAttr::ZINDEX, "0");
    rootWidget.SetAttr(UiAttr::OPACITY, "1.0");
    rootWidget.SetAttr(UiAttr::BACKGROUNDCOLOR, "#FFFFFF");
    rootWidget.SetAttr(UiAttr::BACKGROUNDIMAGE, "");
    rootWidget.SetAttr(UiAttr::BLUR, "0.0");
    rootWidget.SetAttr(UiAttr::HITTESTBEHAVIOR, "default");
    rootWidget.SetAttr(UiAttr::CLIP, "true");
    rootWidget.SetAttr(UiAttr::ORIGINALTEXT, "Root");
    rootWidget.SetAttr(UiAttr::HINT, "Root Hint");
    rootWidget.SetAttr(UiAttr::VISIBLE, "true");
    rootWidget.SetAttr(UiAttr::HASHCODE, "123456");
    rootWidget.SetBounds(Rect(0, 100, 0, 200));
    allWidget.push_back(move(rootWidget));
    Widget childWidget("ROOT,0");
    childWidget.SetAttr(UiAttr::ACCESSIBILITY_ID, "child_accessibility");
    childWidget.SetAttr(UiAttr::ID, "child_id");
    childWidget.SetAttr(UiAttr::TEXT, "ChildButton");
    childWidget.SetAttr(UiAttr::KEY, "child_key");
    childWidget.SetAttr(UiAttr::DESCRIPTION, "Child Description");
    childWidget.SetAttr(UiAttr::TYPE, "Button");
    childWidget.SetAttr(UiAttr::ENABLED, "true");
    childWidget.SetAttr(UiAttr::FOCUSED, "false");
    childWidget.SetAttr(UiAttr::SELECTED, "false");
    childWidget.SetAttr(UiAttr::CLICKABLE, "true");
    childWidget.SetAttr(UiAttr::LONG_CLICKABLE, "true");
    childWidget.SetAttr(UiAttr::SCROLLABLE, "false");
    childWidget.SetAttr(UiAttr::CHECKABLE, "false");
    childWidget.SetAttr(UiAttr::CHECKED, "false");
    childWidget.SetAttr(UiAttr::HOST_WINDOW_ID, "100");
    childWidget.SetAttr(UiAttr::DISPLAY_ID, "0");
    childWidget.SetAttr(UiAttr::ZINDEX, "1");
    childWidget.SetAttr(UiAttr::OPACITY, "1.0");
    childWidget.SetAttr(UiAttr::BACKGROUNDCOLOR, "#FF0000");
    childWidget.SetAttr(UiAttr::BACKGROUNDIMAGE, "");
    childWidget.SetAttr(UiAttr::BLUR, "0.0");
    childWidget.SetAttr(UiAttr::HITTESTBEHAVIOR, "default");
    childWidget.SetAttr(UiAttr::CLIP, "false");
    childWidget.SetAttr(UiAttr::ORIGINALTEXT, "Button");
    childWidget.SetAttr(UiAttr::HINT, "Click me");
    childWidget.SetAttr(UiAttr::VISIBLE, "true");
    childWidget.SetAttr(UiAttr::HASHCODE, "789012");
    childWidget.SetBounds(Rect(10, 90, 10, 50));
    allWidget.push_back(move(childWidget));
    nlohmann::json rootJson;
    DumpOption option;
    option.extendedAttrs_ = "";
    DumpHandler::DumpWindowInfoToJson(option, allWidget, rootJson);
    ASSERT_TRUE(rootJson.contains("attributes")) << "Missing 'attributes' field";
    ASSERT_TRUE(rootJson.contains("children")) << "Missing 'children' field";
    auto& rootAttrs = rootJson["attributes"];
    std::vector<std::string> expectedBaseFields = {
        "accessibilityId", "id", "text", "key", "description", "type",
        "bounds", "enabled", "focused", "selected", "clickable",
        "longClickable", "scrollable", "checkable", "checked",
        "hostWindowId", "displayId", "origBounds", "zIndex",
        "opacity", "backgroundColor", "backgroundImage", "blur",
        "hitTestBehavior", "clip", "originalText", "hierarchy"
    };
    for (const auto& field : expectedBaseFields) {
        ASSERT_TRUE(rootAttrs.contains(field)) << "Missing field in root: " << field;
    }
    ASSERT_TRUE(rootAttrs.contains("visible")) << "Missing field: visible";
    ASSERT_TRUE(rootAttrs.contains("hashcode")) << "Missing field: hashcode";
    ASSERT_TRUE(rootAttrs.contains("hint")) << "Missing field: hint";
    ASSERT_EQ(rootAttrs["accessibilityId"], "root_accessibility");
    ASSERT_EQ(rootAttrs["text"], "RootWidget");
    ASSERT_EQ(rootAttrs["id"], "root_id");
    ASSERT_EQ(rootAttrs["type"], "Window");
    ASSERT_EQ(rootAttrs["bounds"], "[0,0][100,200]");
    ASSERT_EQ(rootAttrs["hierarchy"], "ROOT");
    ASSERT_EQ(rootAttrs["enabled"], "true");
    ASSERT_EQ(rootAttrs["visible"], "true");
    ASSERT_EQ(rootAttrs["hashcode"], "123456");
    ASSERT_EQ(rootAttrs["hint"], "Root Hint");
    auto& children = rootJson["children"];
    ASSERT_TRUE(children.is_array());
    ASSERT_EQ(children.size(), 1);

    auto& childAttrs = children[0]["attributes"];
    for (const auto& field : expectedBaseFields) {
        ASSERT_TRUE(childAttrs.contains(field)) << "Missing field in child: " << field;
    }
    ASSERT_TRUE(childAttrs.contains("visible")) << "Missing field: visible in child";
    ASSERT_TRUE(childAttrs.contains("hashcode")) << "Missing field: hashcode in child";
    ASSERT_TRUE(childAttrs.contains("hint")) << "Missing field: hint in child";
    ASSERT_EQ(childAttrs["accessibilityId"], "child_accessibility");
    ASSERT_EQ(childAttrs["text"], "ChildButton");
    ASSERT_EQ(childAttrs["type"], "Button");
    ASSERT_EQ(childAttrs["bounds"], "[10,10][90,50]");
    ASSERT_EQ(childAttrs["visible"], "true");
    ASSERT_EQ(childAttrs["hashcode"], "789012");
    allWidget[0].SetAttr(UiAttr::UNIQUEID, "unique_root_001");
    allWidget[0].SetAttr(UiAttr::BOUNDSCENTER, "50,100");
    nlohmann::json jsonWithExtended;
    DumpOption optionWithExtended;
    optionWithExtended.extendedAttrs_ = "uniqueId,boundsCenter";
    DumpHandler::DumpWindowInfoToJson(optionWithExtended, allWidget, jsonWithExtended);
    auto& rootAttrsExtended = jsonWithExtended["attributes"];
    ASSERT_TRUE(rootAttrsExtended.contains("uniqueId"));
    ASSERT_EQ(rootAttrsExtended["uniqueId"], "unique_root_001");
    nlohmann::json jsonWithoutExtended;
    DumpOption optionWithoutExtended;
    optionWithoutExtended.extendedAttrs_ = "";
    DumpHandler::DumpWindowInfoToJson(optionWithoutExtended, allWidget, jsonWithoutExtended);
    auto& rootAttrsNoExtended = jsonWithoutExtended["attributes"];
    ASSERT_TRUE(rootAttrsNoExtended.contains("hashcode"));
}
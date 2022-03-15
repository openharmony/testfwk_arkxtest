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

#include "ui_model.h"
#include "gtest/gtest.h"

using namespace OHOS::uitest;
using namespace std;

static constexpr auto ATTR_TEXT = "text";
static constexpr auto ATTR_ID = "id";

TEST(UiModelTest, testRectBase)
{
    Rect rect(100, 200, 300, 400);
    ASSERT_EQ(100, rect.left_);
    ASSERT_EQ(200, rect.right_);
    ASSERT_EQ(300, rect.top_);
    ASSERT_EQ(400, rect.bottom_);

    ASSERT_EQ(rect.GetCenterX(), (100 + 200) / 2);
    ASSERT_EQ(rect.GetCenterY(), (300 + 400) / 2);
}

TEST(UiModelTest, testRectOverlappingDimensions)
{
    Rect rect0(100, 200, 300, 400);
    Rect rect1(0, 100, 0, 100);
    Rect rect2(200, 300, 400, 500);
    Rect rect3(100, 150, 200, 350);
    Rect rect4(150, 250, 350, 450);
    Rect rect5(120, 180, 320, 380);

    int32_t dx = 0, dy = 0;
    rect0.ComputeOverlappingDimensions(rect1, dx, dy); // no overlap
    ASSERT_EQ(0, dx);
    ASSERT_EQ(0, dy);
    rect0.ComputeOverlappingDimensions(rect2, dx, dy); // no overlap
    ASSERT_EQ(0, dx);
    ASSERT_EQ(0, dy);
    rect0.ComputeOverlappingDimensions(rect3, dx, dy); // x,y-overlap
    ASSERT_EQ(50, dx);
    ASSERT_EQ(50, dy);
    rect0.ComputeOverlappingDimensions(rect4, dx, dy); // x,y-overlap
    ASSERT_EQ(50, dx);
    ASSERT_EQ(50, dy);
    rect0.ComputeOverlappingDimensions(rect5, dx, dy); // fully contained
    ASSERT_EQ(60, dx);
    ASSERT_EQ(60, dy);
}

TEST(UiModelTest, testWidgetAttributes)
{
    Widget widget("hierarchy");
    // get not-exist attribute, should return default value
    ASSERT_EQ("none", widget.GetAttr(ATTR_TEXT, "none"));
    // get exist attribute, should return actual value
    widget.SetAttr(ATTR_TEXT, "wyz");
    ASSERT_EQ("wyz", widget.GetAttr(ATTR_TEXT, "none"));
}

/** NOTE:: Widget need to be movable since we need to move a constructed widget to
 * its hosting tree. We must ensure that the move-created widget be same as the
 * moved one (No any attribute/filed should be lost during moving).
 * */
TEST(UiModelTest, testWidgetSafeMovable)
{
    Widget widget("hierarchy");
    widget.SetAttr(ATTR_TEXT, "wyz");
    widget.SetAttr(ATTR_ID, "100");
    widget.SetHostTreeId("tree-10086");
    widget.SetBounds(1, 2, 3, 4);

    auto newWidget = move(widget);
    ASSERT_EQ("hierarchy", newWidget.GetHierarchy());
    ASSERT_EQ("tree-10086", newWidget.GetHostTreeId());
    ASSERT_EQ("wyz", newWidget.GetAttr(ATTR_TEXT, ""));
    ASSERT_EQ("100", newWidget.GetAttr(ATTR_ID, ""));
    auto bounds = newWidget.GetBounds();
    ASSERT_EQ(1, bounds.left_);
    ASSERT_EQ(2, bounds.right_);
    ASSERT_EQ(3, bounds.top_);
    ASSERT_EQ(4, bounds.bottom_);
}

TEST(UiModelTest, testWidgetToStr)
{
    Widget widget("hierarchy");

    // get exist attribute, should return default value
    widget.SetAttr(ATTR_TEXT, "wyz");
    widget.SetAttr(ATTR_ID, "100");

    auto str0 = widget.ToStr();
    ASSERT_TRUE(str0.find(ATTR_TEXT) != string::npos);
    ASSERT_TRUE(str0.find("wyz") != string::npos);
    ASSERT_TRUE(str0.find(ATTR_ID) != string::npos);
    ASSERT_TRUE(str0.find("100") != string::npos);

    // change attribute
    widget.SetAttr(ATTR_ID, "211");
    str0 = widget.ToStr();
    ASSERT_TRUE(str0.find(ATTR_TEXT) != string::npos);
    ASSERT_TRUE(str0.find("wyz") != string::npos);
    ASSERT_TRUE(str0.find(ATTR_ID) != string::npos);
    ASSERT_TRUE(str0.find("211") != string::npos);
    ASSERT_TRUE(str0.find("100") == string::npos);
}

// define a widget visitor to visit the specified attribute
class WidgetAttrVisitor : public WidgetVisitor {
public:
    explicit WidgetAttrVisitor(string_view attr) : attr_(attr) {}

    ~WidgetAttrVisitor() {}

    void Visit(const Widget &widget) override
    {
        if (!firstWidget_) {
            attrValueSequence_ << ",";
        } else {
            firstWidget_ = false;
        }
        attrValueSequence_ << widget.GetAttr(attr_, "");
    }

    stringstream attrValueSequence_;

private:
    const string attr_;
    volatile bool firstWidget_ = true;
};

TEST(UiModelTest, testConstructWidgetsFromDamagedDom)
{
    WidgetTree tree("tree");
    stringstream errorReceiver;
    ASSERT_FALSE(tree.ConstructFromDomText("abc", errorReceiver)) << "Construct from damaged dom should fail";
}

TEST(UiModelTest, testConstructWidgetsFromDom)
{
    static constexpr string_view domText = R"({
"attributes": {
"index": "0",
"resource-id": "id0",
"text": ""},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id00",
"text": ""},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id000",
"text": ""},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id0000",
"text": "USB"},
"children": []
}]}]},
{
"attributes": {
"index": "1",
"resource-id": "id01",
"text": ""},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id010",
"text": ""},
"children": []
}]}]})";

    stringstream errorReceiver;
    WidgetTree tree("tree");
    ASSERT_TRUE(tree.ConstructFromDomText(domText, errorReceiver));

    // visited the widget tree and check the 'resource-id' attribute
    WidgetAttrVisitor visitor("resource-id");
    tree.DfsTraverse(visitor);
    // should collect correct attribute value sequence (DFS)
    ASSERT_EQ("id0,id00,id000,id0000,id01,id010", visitor.attrValueSequence_.str()) << "Incorrect node order";
}

class BoundsVisitor : public WidgetVisitor {
public:
    void Visit(const Widget &widget) override
    {
        bounds_ = widget.GetBounds();
    }

    Rect bounds_{0, 0, 0, 0};
};

TEST(UiModelTest, testConstructWidgetsFromDomCheckBounds)
{
    constexpr string_view domText = R"({
"attributes": {
"rotation": "0"
},
"children": [
{
"attributes": {
"bounds": "[120,960][1437,1518]",
"index": "0",
"resource-id": "id4",
"text": "Use USB to"
},
"children": []
}
]
}
)";

    stringstream errorReceiver;
    WidgetTree tree("tree");
    ASSERT_TRUE(tree.ConstructFromDomText(domText, errorReceiver));
    BoundsVisitor visitor;
    tree.DfsTraverse(visitor);
    Rect bounds = visitor.bounds_;
    ASSERT_EQ(120, bounds.left_);
    ASSERT_EQ(1437, bounds.right_);
    ASSERT_EQ(960, bounds.top_);
    ASSERT_EQ(1518, bounds.bottom_);
}

TEST(UiModelTest, testGetRelativesNode)
{
    constexpr string_view domText = R"({
"attributes": {
"index": "0",
"resource-id": "id2",
"text": "zl"
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "Use USB to"
},
"children": []
},
{
"attributes": {
"index": "1",
"resource-id": "id4",
"text": "WYZ"
},
"children": []
}
]
}
)";

    stringstream errorReceiver;
    WidgetTree tree("tree");
    ASSERT_TRUE(tree.ConstructFromDomText(domText, errorReceiver));
    BoundsVisitor visitor;
    tree.DfsTraverse(visitor);

    auto rootPtr = tree.GetRootWidget();
    ASSERT_TRUE(rootPtr != nullptr) << "Failed to get root node";
    ASSERT_EQ(nullptr, tree.GetParentWidget(*rootPtr)) << "Root node should have no parent";
    ASSERT_EQ("zl", rootPtr->GetAttr("text", "")) << "Incorrect root node attribute";

    auto child1Ptr = tree.GetChildWidget(*rootPtr, 1);
    ASSERT_TRUE(child1Ptr != nullptr) << "Failed to get child widget of root node at index 1";
    ASSERT_EQ("WYZ", child1Ptr->GetAttr("text", "")) << "Incorrect child node attribute";

    ASSERT_TRUE(tree.GetChildWidget(*rootPtr, 2) == nullptr)
                                << "Get child widget of root node at index 2 should returns null";
}

TEST(UiModelTest, testVisitNodesInGivenRoot)
{
    constexpr string_view domText = R"({
"attributes": {
"index": "0",
"resource-id": "id2",
"text": "zl"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "people"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "wlj"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "wyz"},
"children": []},
{
"attributes": {
"index": "1",
"resource-id": "id4",
"text": "meco"},
"children": []}]}]},
{
"attributes": {
"index": "1",
"resource-id": "id4",
"text": "WYZ"},
"children": []}]})";

    stringstream errorReceiver;
    WidgetTree tree("tree");
    ASSERT_TRUE(tree.ConstructFromDomText(domText, errorReceiver));

    auto rootPtr = tree.GetRootWidget();
    ASSERT_TRUE(rootPtr != nullptr) << "Failed to get root node";
    ASSERT_EQ(nullptr, tree.GetParentWidget(*rootPtr)) << "Root node should have no parent";

    auto child0Ptr = tree.GetChildWidget(*rootPtr, 0);
    ASSERT_TRUE(child0Ptr != nullptr) << "Failed to get child widget of root node at index 0";

    WidgetAttrVisitor attrVisitor("text");
    tree.DfsTraverseDescendants(attrVisitor, *child0Ptr);
    auto visitedTextSequence = attrVisitor.attrValueSequence_.str();
    ASSERT_EQ("people,wlj,wyz,meco", visitedTextSequence) << "Incorrect text sequence of node descendants";
}

TEST(UiModelTest, testVisitFrontNodes)
{
    constexpr string_view domText = R"({
"attributes": {
"index": "0",
"resource-id": "id2",
"text": "zl"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "people"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "wlj"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "wyz"},
"children": []},
{
"attributes": {
"index": "1",
"resource-id": "id4",
"text": "meco"},
"children": []}]}]},
{
"attributes": {
"index": "1",
"resource-id": "id4",
"text": "WYZ"},
"children": []}]})";

    stringstream errorReceiver;
    WidgetTree tree("tree");
    ASSERT_TRUE(tree.ConstructFromDomText(domText, errorReceiver));

    auto rootPtr = tree.GetRootWidget();
    ASSERT_TRUE(rootPtr != nullptr) << "Failed to get root node";
    ASSERT_EQ(nullptr, tree.GetParentWidget(*rootPtr)) << "Root node should have no parent";

    auto child0Ptr = tree.GetChildWidget(*rootPtr, 0);
    ASSERT_TRUE(child0Ptr != nullptr) << "Failed to get child widget of root node at index 0";

    WidgetAttrVisitor attrVisitor("text");
    tree.DfsTraverseFronts(attrVisitor, *child0Ptr);
    auto visitedTextSequence = attrVisitor.attrValueSequence_.str();
    ASSERT_EQ("zl", visitedTextSequence) << "Incorrect text sequence of front nodes";
}

TEST(UiModelTest, testVisitTearNodes)
{
    constexpr string_view domText = R"({
"attributes": {
"index": "0",
"resource-id": "id2",
"text": "zl"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "people"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "wlj"},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "wyz"},
"children": []},
{
"attributes": {
"index": "1",
"resource-id": "id4",
"text": "meco"},
"children": []}]}]},
{
"attributes": {
"index": "1",
"resource-id": "id4",
"text": "WYZ"},
"children": []}]})";

    stringstream errorReceiver;
    WidgetTree tree("tree");
    ASSERT_TRUE(tree.ConstructFromDomText(domText, errorReceiver));

    auto rootPtr = tree.GetRootWidget();
    ASSERT_TRUE(rootPtr != nullptr) << "Failed to get root node";
    ASSERT_EQ(nullptr, tree.GetParentWidget(*rootPtr)) << "Root node should have no parent";

    auto child0Ptr = tree.GetChildWidget(*rootPtr, 0);
    ASSERT_TRUE(child0Ptr != nullptr) << "Failed to get child widget of root node at index 0";

    WidgetAttrVisitor attrVisitor("text");
    tree.DfsTraverseRears(attrVisitor, *child0Ptr);
    auto visitedTextSequence = attrVisitor.attrValueSequence_.str();
    ASSERT_EQ("wlj,wyz,meco,WYZ", visitedTextSequence) << "Incorrect text sequence of tear nodes";
}
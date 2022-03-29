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
#include "ui_model.h"

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

TEST(UiModelTest, testRectIntersection)
{
    Rect rect0(100, 200, 300, 400);
    Rect rect1(0, 100, 0, 100);
    Rect rect2(200, 300, 400, 500);
    Rect rect3(100, 150, 200, 350);
    Rect rect4(150, 250, 350, 450);
    Rect rect5(120, 180, 320, 380);

    Rect intersection {0, 0, 0, 0};
    ASSERT_FALSE(rect0.ComputeIntersection(rect1, intersection)); // no overlap
    ASSERT_FALSE(rect0.ComputeIntersection(rect2, intersection)); // no overlap
    ASSERT_TRUE(rect0.ComputeIntersection(rect3, intersection)); // x,y-overlap
    ASSERT_EQ(100, intersection.left_);
    ASSERT_EQ(150, intersection.right_);
    ASSERT_EQ(300, intersection.top_);
    ASSERT_EQ(350, intersection.bottom_);
    intersection = {0, 0, 0, 0};
    ASSERT_TRUE(rect0.ComputeIntersection(rect4, intersection)); // x,y-overlap
    ASSERT_EQ(150, intersection.left_);
    ASSERT_EQ(200, intersection.right_);
    ASSERT_EQ(350, intersection.top_);
    ASSERT_EQ(400, intersection.bottom_);
    intersection = {0, 0, 0, 0};
    ASSERT_TRUE(rect0.ComputeIntersection(rect5, intersection)); // fully contained
    ASSERT_EQ(120, intersection.left_);
    ASSERT_EQ(180, intersection.right_);
    ASSERT_EQ(320, intersection.top_);
    ASSERT_EQ(380, intersection.bottom_);
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

    static constexpr string_view DOM_TEXT = R"({
"attributes": {"resource-id": "id0"},
"children": [
{
"attributes": {"resource-id": "id00"},
"children": [
{
"attributes": {"resource-id": "id000"},
"children": [
{
"attributes": {"resource-id": "id0000"},
"children": []
}]}]},
{
"attributes": {"resource-id": "id01"},
"children": [
{
"attributes": {"resource-id": "id010"},
"children": []
}]}]})";

TEST(UiModelTest, testConstructWidgetsFromDomCheckOrder)
{
    auto dom = nlohmann::json::parse(DOM_TEXT);
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, false);

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
        boundsList_.emplace_back(widget.GetBounds());
    }

    vector<Rect> boundsList_;
};

TEST(UiModelTest, testConstructWidgetsFromDomCheckBounds)
{
    auto dom = nlohmann::json::parse(R"({"attributes":{"bounds":"[0,-50][100,200]"},"children":[]})");
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, false);
    BoundsVisitor visitor;
    tree.DfsTraverse(visitor);
    auto& bounds = visitor.boundsList_.at(0);
    ASSERT_EQ(0, bounds.left_);
    ASSERT_EQ(100, bounds.right_); // check converting negative number
    ASSERT_EQ(-50, bounds.top_);
    ASSERT_EQ(200, bounds.bottom_);
}

TEST(UiModelTest, testGetRelativeNode)
{
    auto dom = nlohmann::json::parse(DOM_TEXT);
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, false);
    BoundsVisitor visitor;
    tree.DfsTraverse(visitor);

    auto rootPtr = tree.GetRootWidget();
    ASSERT_TRUE(rootPtr != nullptr) << "Failed to get root node";
    ASSERT_EQ(nullptr, tree.GetParentWidget(*rootPtr)) << "Root node should have no parent";
    ASSERT_EQ("id0", rootPtr->GetAttr("resource-id", "")) << "Incorrect root node attribute";

    auto child1Ptr = tree.GetChildWidget(*rootPtr, 1);
    ASSERT_TRUE(child1Ptr != nullptr) << "Failed to get child widget of root node at index 1";
    ASSERT_EQ("id01", child1Ptr->GetAttr("resource-id", "")) << "Incorrect child node attribute";

    ASSERT_TRUE(tree.GetChildWidget(*rootPtr, 2) == nullptr) << "Unexpected child not";
}

TEST(UiModelTest, testVisitNodesInGivenRoot)
{
    auto dom = nlohmann::json::parse(DOM_TEXT);
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, false);

    auto rootPtr = tree.GetRootWidget();
    ASSERT_TRUE(rootPtr != nullptr) << "Failed to get root node";
    ASSERT_EQ(nullptr, tree.GetParentWidget(*rootPtr)) << "Root node should have no parent";

    auto child0Ptr = tree.GetChildWidget(*rootPtr, 0);
    ASSERT_TRUE(child0Ptr != nullptr) << "Failed to get child widget of root node at index 0";

    WidgetAttrVisitor attrVisitor("resource-id");
    tree.DfsTraverseDescendants(attrVisitor, *child0Ptr);
    auto visitedTextSequence = attrVisitor.attrValueSequence_.str();
    ASSERT_EQ("id00,id000,id0000", visitedTextSequence) << "Incorrect text sequence of node descendants";
}

TEST(UiModelTest, testVisitFrontNodes)
{
    auto dom = nlohmann::json::parse(DOM_TEXT);
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, false);

    auto rootPtr = tree.GetRootWidget();
    ASSERT_TRUE(rootPtr != nullptr) << "Failed to get root node";
    ASSERT_EQ(nullptr, tree.GetParentWidget(*rootPtr)) << "Root node should have no parent";

    auto child1Ptr = tree.GetChildWidget(*rootPtr, 1);
    ASSERT_TRUE(child1Ptr != nullptr) << "Failed to get child widget of root node at index 0";

    WidgetAttrVisitor attrVisitor("resource-id");
    tree.DfsTraverseFronts(attrVisitor, *child1Ptr);
    auto visitedTextSequence = attrVisitor.attrValueSequence_.str();
    ASSERT_EQ("id0,id00,id000,id0000", visitedTextSequence) << "Incorrect text sequence of front nodes";
}

TEST(UiModelTest, testVisitTearNodes)
{
    auto dom = nlohmann::json::parse(DOM_TEXT);
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, false);

    auto rootPtr = tree.GetRootWidget();
    ASSERT_TRUE(rootPtr != nullptr) << "Failed to get root node";
    ASSERT_EQ(nullptr, tree.GetParentWidget(*rootPtr)) << "Root node should have no parent";

    auto child0Ptr = tree.GetChildWidget(*rootPtr, 0);
    ASSERT_TRUE(child0Ptr != nullptr) << "Failed to get child widget of root node at index 0";

    WidgetAttrVisitor attrVisitor("resource-id");
    tree.DfsTraverseRears(attrVisitor, *child0Ptr);
    auto visitedTextSequence = attrVisitor.attrValueSequence_.str();
    ASSERT_EQ("id000,id0000,id01,id010", visitedTextSequence) << "Incorrect text sequence of tear nodes";
}

TEST(UiModelTest, testBoundsAndVisibilityCorrection)
{
    constexpr string_view domText = R"(
{"attributes": {"resource-id": "id0","bounds": "[0,0][100,100]"},
"children": [
{"attributes": {"resource-id": "id00","bounds": "[0,20][100,80]"},
"children": [ {"attributes": {"resource-id": "id000","bounds": "[0,10][100,90]"}, "children": []} ]
},
{"attributes": {"resource-id": "id01","bounds": "[0,-20][100,100]"},
"children": [ {"attributes": {"resource-id": "id010","bounds": "[0,-20][100,0]"}, "children": []},
{"attributes": {"resource-id": "id011","bounds": "[0,-20][100,20]"}, "children": []}]
}
]
})";
//                  id01 id010 id011
//                   |    |     |
//                   |    |     |
//  id0              |    |     |
//   |        id000  |          |  
//   |   id00  |     |          |
//   |    |    |     |
//   |    |    |     |
//   |    |    |     |
//   |         |     |
//   |               |
    auto dom = nlohmann::json::parse(domText);
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, true); // enable bounds amending
    WidgetAttrVisitor attrVisitor("resource-id");
    tree.DfsTraverse(attrVisitor);
    // id010 should be discarded dut to totaly invisible
    ASSERT_EQ("id0,id00,id000,id01,id011", attrVisitor.attrValueSequence_.str());
    BoundsVisitor boundsVisitor;
    tree.DfsTraverse(boundsVisitor);
    // check revised bounds
    vector<Rect> expectedBounds = { Rect {0, 100, 0, 100}, Rect {0, 100, 20, 80},
        Rect {0, 100, 20, 80}, Rect {0, 100, 0, 100}, Rect {0, 100, 0, 20} };
    ASSERT_EQ(expectedBounds.size(), boundsVisitor.boundsList_.size());
    for (auto index = 0; index < expectedBounds.size(); index++) {
        auto& expectedBound = expectedBounds.at(index);
        auto& actualBound = boundsVisitor.boundsList_.at(index);
        ASSERT_EQ(expectedBound.left_, actualBound.left_);
        ASSERT_EQ(expectedBound.right_, actualBound.right_);
        ASSERT_EQ(expectedBound.top_, actualBound.top_);
        ASSERT_EQ(expectedBound.bottom_, actualBound.bottom_);
    }
}
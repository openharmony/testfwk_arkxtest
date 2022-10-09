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
    ASSERT_EQ("none", widget.GetAttr(ATTR_TEXT, "none"));
    // get exist attribute, should return actual value
    widget.SetAttr(ATTR_TEXT, "wyz");
    ASSERT_EQ("wyz", widget.GetAttr(ATTR_TEXT, "none"));
}

/** NOTE:: Widget need to be movable since we need to move a constructed widget to
 * its hosting tree. We must ensure that the move-created widget be same as the
 * moved one (No any attribute/filed should be lost during moving).
 * */
TEST(WidgetTest, testSafeMovable)
{
    Widget widget("hierarchy");
    widget.SetAttr(ATTR_TEXT, "wyz");
    widget.SetAttr(ATTR_ID, "100");
    widget.SetHostTreeId("tree-10086");
    widget.SetBounds(Rect(1, 2, 3, 4));

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

TEST(WidgetTest, testToStr)
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

TEST(WidgetTreeTest, testConstructWidgetsFromDomCheckOrder)
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

TEST(WidgetTreeTest, testConstructWidgetsFromDomCheckBounds)
{
    auto dom = nlohmann::json::parse(R"({"attributes":{"bounds":"[0,-50][100,200]"}, "children":[]})");
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, false);
    BoundsVisitor visitor;
    tree.DfsTraverse(visitor);
    auto &bounds = visitor.boundsList_.at(0);
    ASSERT_EQ(0, bounds.left_);
    ASSERT_EQ(100, bounds.right_); // check converting negative number
    ASSERT_EQ(-50, bounds.top_);
    ASSERT_EQ(200, bounds.bottom_);
}

TEST(WidgetTreeTest, testGetRelativeNode)
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

TEST(WidgetTreeTest, testVisitNodesInGivenRoot)
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

TEST(WidgetTreeTest, testVisitFrontNodes)
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

TEST(WidgetTreeTest, testVisitTearNodes)
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

TEST(WidgetTreeTest, testBoundsAndVisibilityCorrection)
{
    constexpr string_view domText = R"(
{"attributes" : {"resource-id" : "id0","bounds" : "[0,0][100,100]"},
"children": [
{"attributes" : { "resource-id" : "id00","bounds" : "[0,20][100,80]" },
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
    vector<Rect> expectedBounds = {Rect(0, 100, 0, 100), Rect(0, 100, 20, 80), Rect(0, 100, 20, 80),
                                   Rect(0, 100, 0, 100), Rect(0, 100, 0, 20)};
    ASSERT_EQ(expectedBounds.size(), boundsVisitor.boundsList_.size());
    for (size_t index = 0; index < expectedBounds.size(); index++) {
        auto &expectedBound = expectedBounds.at(index);
        auto &actualBound = boundsVisitor.boundsList_.at(index);
        ASSERT_EQ(expectedBound.left_, actualBound.left_);
        ASSERT_EQ(expectedBound.right_, actualBound.right_);
        ASSERT_EQ(expectedBound.top_, actualBound.top_);
        ASSERT_EQ(expectedBound.bottom_, actualBound.bottom_);
    }
}

TEST(WidgetTreeTest, testMarshalIntoDom)
{
    auto dom = nlohmann::json::parse(DOM_TEXT);
    WidgetTree tree("tree");
    tree.ConstructFromDom(dom, false);
    auto dom1 = nlohmann::json();
    tree.MarshalIntoDom(dom1);
    ASSERT_FALSE(dom1.empty());
}

/** Make merged tree from doms and collects the target attribute dfs sequence.*/
static void CheckMergedTree(const array<string_view, 3> &doms, map<string, string> &attrCollector)
{
    WidgetTree tree("");
    vector<unique_ptr<WidgetTree>> subTrees;
    for (auto &dom : doms) {
        auto tempTree = make_unique<WidgetTree>("");
        tempTree->ConstructFromDom(nlohmann::json::parse(dom), true);
        subTrees.push_back(move(tempTree));
    }
    WidgetTree::MergeTrees(subTrees, tree);
    for (auto &[name, value] : attrCollector) {
        if (name == "bounds") {
            BoundsVisitor visitor;
            tree.DfsTraverse(visitor);
            stringstream stream;
            for (auto &rect : visitor.boundsList_) {
                stream << "[" << rect.left_ << "," << rect.top_ << "][" << rect.right_ << "," << rect.bottom_ << "]";
                stream << ",";
            }
            attrCollector[name] = stream.str();
        } else {
            WidgetAttrVisitor visitor(name);
            tree.DfsTraverse(visitor);
            attrCollector[name] = visitor.attrValueSequence_.str();
        }
    }
}

TEST(WidgetTreeTest, testMergeTreesNoIntersection)
{
    // 3 tree vertical/horizontal arranged without intersection
    constexpr string_view domText0 = R"(
{
"attributes": {"id": "t0-id0", "bounds": "[0,0][2,2]"},
"children": [{"attributes": {"id": "t0-id00", "bounds": "[0,0][2,1]"}, "children": []}]
})";
    constexpr string_view domText1 = R"(
{
"attributes": {"id": "t1-id0", "bounds": "[0,2][2,4]"},
"children": [{"attributes": {"id": "t1-id00", "bounds": "[0,2][2,3]"}, "children": []}]
})";
    constexpr string_view domText2 = R"(
{
"attributes": {"id": "t2-id0", "bounds": "[2,0][4,4]"},
"children": [{"attributes": {"id": "t2-id00", "bounds": "[2,0][4,3]"}, "children": []}]
})";
    map<string, string> attrs;
    attrs["id"] = "";
    attrs["bounds"] = "";
    CheckMergedTree( {domText0, domText1, domText2}, attrs);
    // all widgets should be available (leading ',': separator of virtual-root node attr-value)
    ASSERT_EQ(",t0-id0,t0-id00,t1-id0,t1-id00,t2-id0,t2-id00", attrs["id"]);
    // bounds should not be revised (leading '[0,0][4,4]': auto-computed virtual-root node bounds)
    ASSERT_EQ("[0,0][4,4],[0,0][2,2],[0,0][2,1],[0,2][2,4],[0,2][2,3],[2,0][4,4],[2,0][4,3],", attrs["bounds"]);
}

TEST(WidgetTreeTest, testMergeTreesWithFullyCovered)
{
    // dom2 is fully covered by dom0 and dom1
    constexpr string_view domText0 = R"(
{
"attributes": {"id": "t0-id0", "bounds": "[0,0][2,2]"},
"children": [{"attributes": {"id": "t0-id00", "bounds": "[0,0][2,1]"}, "children": []}]
})";
    constexpr string_view domText1 = R"(
{
"attributes": {"id": "t1-id0", "bounds": "[0,2][2,4]"},
"children": [{"attributes": {"id": "t1-id00", "bounds": "[0,2][2,3]"}, "children": []}]
})";
    constexpr string_view domText2 = R"(
{
"attributes": {"id": "t2-id0", "bounds": "[0,0][2,4]"},
"children": [{"attributes": {"id": "t2-id00", "bounds": "[2,0][4,3]"}, "children": []}]
})";
    map<string, string> attrs;
    attrs["id"] = "";
    attrs["bounds"] = "";
    CheckMergedTree( {domText0, domText1, domText2}, attrs);
    // tree2 widgets should be discarded (leading ',': separator of virtual-root node attr-value)
    ASSERT_EQ(",t0-id0,t0-id00,t1-id0,t1-id00", attrs["id"]);
    // bounds should not be revised (leading '[0,0][2,4]': auto-computed virtual-root node bounds)
    ASSERT_EQ("[0,0][2,4],[0,0][2,2],[0,0][2,1],[0,2][2,4],[0,2][2,3],", attrs["bounds"]);
}

TEST(WidgetTreeTest, testMergeTreesWithPartialCovered)
{
    constexpr string_view domText0 = R"(
{
"attributes": {"id": "t0-id0", "bounds": "[0,0][4,4]"},
"children": [{"attributes": {"id": "t0-id00", "bounds": "[0,0][4,2]"}, "children": []}]
})";
    // t1-id0 is partial covered by tree0, t1-id00 is fully covered
    constexpr string_view domText1 = R"(
{
"attributes": {"id": "t1-id0", "bounds": "[0,2][4,6]"},
"children": [{"attributes": {"id": "t1-id00", "bounds": "[0,2][4,4]"}, "children": []}]
})";
    // t2-id0 is partial covered by tree0/tree1, t2-id00 is fully covered by tree0/tree1
    constexpr string_view domText2 = R"(
{
"attributes": {"id": "t2-id0", "bounds": "[0,0][4,8]"},
"children": [{"attributes": {"id": "t2-id00", "bounds": "[0,0][4,5]"}, "children": []}]
})";
    map<string, string> attrs;
    attrs["id"] = "";
    attrs["bounds"] = "";
    CheckMergedTree( {domText0, domText1, domText2}, attrs);
    // check visible widgets (leading ',': separator of virtual-root node attr-value)
    ASSERT_EQ(",t0-id0,t0-id00,t1-id0,t2-id0", attrs["id"]);
    // bounds should not be revised (leading '[0,0][2,4]': auto-computed virtual-root node bounds)
    // t1-id0: [0,4][4,6]; t2-id0: [0,6][4,8]
    ASSERT_EQ("[0,0][4,8],[0,0][4,4],[0,0][4,2],[0,4][4,6],[0,6][4,8],", attrs["bounds"]);
}
/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "dom_parser.h"
#include "gtest/gtest.h"

using namespace OHOS::uitest;
using namespace std;

static constexpr char HIERARCHY_ROOT_NODE[] = "root";

static constexpr char DOM_TEXT[] = R"(
{
"attributes": {
"index": "0",
"resource-id": "id1",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id2",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id3",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id4",
"text": "USB"
},
"children": []
}
]
}
]
},
{
"attributes": {
"index": "1",
"resource-id": "id5",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id6",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id7",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id8",
"text": "Photos"
},
"children": []
}
]
},
{
"attributes": {
"index": "1",
"resource-id": "id9",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id10",
"text": "Files"
},
"children": []
}
]
},
{
"attributes": {
"index": "2",
"resource-id": "id11",
"text": ""
},
"children": [
{
"attributes": {
"index": "0",
"resource-id": "id12",
"text": "Charge"
},
"children": []
}
]
}
]
}
]
},
{
"attributes": {
"index": "0",
"resource-id": "id13",
"text": "CANCEL"
},
"children": []
}
]
})";

class MockDomVisitor : public DomNodeVisitor {
public:
    void Visit(string_view tag, string_view hierarchy,
               const map<string, string> &attributes) override
    {
        nodes_.insert(make_pair(hierarchy, attributes));
        visitationTrace_.push_back(string(hierarchy));
    }

    void OnDomParseError(string_view message) override
    {
        errorRaised_ = true;
    }

    bool GetNodeAttributesByHierarchy(string_view hierarchy, map<string, string> &receiver)
    {
        auto find = nodes_.find(string(hierarchy));
        if (find == nodes_.end()) {
            return false;
        }
        auto node = find->second;
        for (auto &it : node) {
            receiver.insert(make_pair(it.first, it.second));
        }
        return true;
    }

    int32_t GetVisitOrderOfNodeByHierarchy(string_view hierarchy)
    {
        auto it = find(visitationTrace_.begin(), visitationTrace_.end(), hierarchy);
        if (it != visitationTrace_.end()) {
            return distance(visitationTrace_.begin(), it);
        } else {
            return -1;
        }
    }

    bool HasPendingError() const
    {
        return errorRaised_;
    }

private:
    // hierarchy --- attr_dict
    map<string, map<string, string>> nodes_;
    // visit sequence of hierarchy
    vector<string> visitationTrace_;
    bool errorRaised_ = false;
};

class MockHierarchyBuilder : public NodeHierarchyBuilder {
public:
    string BuildForRootWidget() const override
    {
        return HIERARCHY_ROOT_NODE;
    }

    string Build(string_view parentNodeHierarchy, uint32_t childIndex) const override
    {
        return string(parentNodeHierarchy) + "," + to_string(childIndex);
    }
};

TEST(DOMTreeTest, parseEmptyDOM)
{
    MockDomVisitor visitor;
    MockHierarchyBuilder builder;
    DomTreeParser::DfsParse("", visitor, builder);
    ASSERT_TRUE(visitor.HasPendingError()) << "Visit empty dom text should cause error";
}

TEST(DOMTreeTest, parseDamagedDOM)
{
    MockDomVisitor visitor;
    MockHierarchyBuilder builder;
    DomTreeParser::DfsParse("wyz", visitor, builder);
    ASSERT_TRUE(visitor.HasPendingError()) << "Visit damaged dom text should cause error";
}

// check the dfs visit order
TEST(DOMTreeTest, nodeVisitOrder)
{
    MockDomVisitor visitor;
    MockHierarchyBuilder builder;
    DomTreeParser::DfsParse(DOM_TEXT, visitor, builder);
    ASSERT_FALSE(visitor.HasPendingError());
    auto constexpr hierarchy1 = "root,0";
    auto constexpr hierarchy2 = "root,1";
    auto constexpr hierarchy11 = "root,0,0";
    auto orderRoot = visitor.GetVisitOrderOfNodeByHierarchy(HIERARCHY_ROOT_NODE);
    auto orderRoot1 = visitor.GetVisitOrderOfNodeByHierarchy(hierarchy1);
    auto orderRoot2 = visitor.GetVisitOrderOfNodeByHierarchy(hierarchy2);
    auto orderRoot11 = visitor.GetVisitOrderOfNodeByHierarchy(hierarchy11);
    ASSERT_EQ(0, orderRoot) << "Root must be the first visited node";
    ASSERT_TRUE(orderRoot1 < orderRoot2) << "Left node must be visited before right node";
    ASSERT_TRUE(orderRoot1 < orderRoot11) << "Parent must be visited before child";
    ASSERT_TRUE(orderRoot11 < orderRoot2) << "Children of left-brother node must be visited first";
}

TEST(DOMTreeTest, dumpNodeAttributes)
{
    MockDomVisitor visitor;
    MockHierarchyBuilder builder;
    DomTreeParser::DfsParse(DOM_TEXT, visitor, builder);
    ASSERT_FALSE(visitor.HasPendingError());

    // the actual hierarchy, must key synchronous with the XML_CONTENT and the hierarchy builder
    static constexpr char hierarchyUsb[] = "root,0,0,0";
    static constexpr char hierarchyPhotos[] = "root,1,0,0,0";
    static constexpr char hierarchyFiles[] = "root,1,0,1,0";

    map<string, string> nodeAttrs;
    // check the correctness of node hierarchy and attributes parsed from dom tree
    ASSERT_TRUE(visitor.GetNodeAttributesByHierarchy(hierarchyUsb, nodeAttrs));
    ASSERT_EQ("USB", nodeAttrs.find("text")->second);

    nodeAttrs.clear();
    ASSERT_TRUE(visitor.GetNodeAttributesByHierarchy(hierarchyPhotos, nodeAttrs));
    ASSERT_EQ("Photos", nodeAttrs.find("text")->second);

    nodeAttrs.clear();
    ASSERT_TRUE(visitor.GetNodeAttributesByHierarchy(hierarchyFiles, nodeAttrs));
    ASSERT_EQ("Files", nodeAttrs.find("text")->second);
}

TEST(DOMTreeTest, dumpDOMTreeNoSuchNode)
{
    MockDomVisitor visitor;
    MockHierarchyBuilder builder;
    DomTreeParser::DfsParse(DOM_TEXT, visitor, builder);
    ASSERT_FALSE(visitor.HasPendingError());

    auto constexpr hierarchy = "WYZ";
    map<string, string> nodeAttrs;
    // given a non-exist node, check the find result
    ASSERT_FALSE(visitor.GetNodeAttributesByHierarchy(hierarchy, nodeAttrs));
    ASSERT_EQ(-1, visitor.GetVisitOrderOfNodeByHierarchy(hierarchy));
}
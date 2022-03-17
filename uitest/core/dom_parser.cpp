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

#include <string>
#include "dom_parser.h"
#include "json.hpp"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;
    static constexpr char TAG_NODE[] = "node";

    static void DfsParseNode(json &root, DomNodeVisitor &visitor, string_view hierarchy,
                             const NodeHierarchyBuilder &builder)
    {
        auto attributesData = root["attributes"];
        auto childrenData = root["children"];
        map<string, string> attributeDict;
        for (auto &item:attributesData.items()) {
            attributeDict[item.key()] = item.value();
        }
        visitor.Visit(TAG_NODE, hierarchy, attributeDict);
        const size_t childCount = childrenData.size();
        for (size_t idx = 0; idx < childCount; idx++) {
            auto &child = childrenData.at(idx);
            auto childHierarchy = builder.Build(string(hierarchy), idx);
            DfsParseNode(child, visitor, childHierarchy, builder);
        }
    }

    void DomTreeParser::DfsParse(string_view content, DomNodeVisitor &visitor, const NodeHierarchyBuilder &builder)
    {
        if (content.empty()) {
            visitor.OnDomParseError("DOM content is empty");
            return;
        }
        try {
            auto domJson = json::parse(content);
            DfsParseNode(domJson, visitor, builder.BuildForRootWidget(), builder);
        } catch (exception &ex) {
            visitor.OnDomParseError(ex.what());
            return;
        }
    }
}

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

#ifndef DOM_PARSER_H
#define DOM_PARSER_H

#include <string>
#include <map>

namespace OHOS::uitest {

    class NodeHierarchyBuilder {
    public:
        virtual std::string BuildForRootWidget() const = 0;

        virtual std::string Build(std::string_view parentNodeHierarchy, uint32_t childIndex) const = 0;
    };

    class DomNodeVisitor {

    public:
        virtual void Visit(std::string_view tag, std::string_view hierarchy,
                           const std::map<std::string, std::string> &attributes) = 0;

        virtual void OnDomParseError(std::string_view message) = 0;
    };

    class DomTreeParser {
    public:
        DomTreeParser() = delete;

        virtual ~DomTreeParser() {}

        static void DfsParse(std::string_view content, DomNodeVisitor &visitor, const NodeHierarchyBuilder &builder);
    };
}

#endif
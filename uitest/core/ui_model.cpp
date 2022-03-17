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

#include <algorithm>
#include "ui_model.h"
#include "dom_parser.h"
#include "common_defines.h"

namespace OHOS::uitest {
    using namespace std;

    static constexpr auto ROOT_HIERARCHY = "ROOT";

    void Rect::ComputeOverlappingDimensions(const Rect &other, int32_t &width, int32_t& height) const
    {
        if (left_ >= other.right_ || right_ <= other.left_) {
            width = 0;
        } else {
            vector<int32_t> px = {left_, right_, other.left_, other.right_};
            sort(px.begin(), px.end());
            width = px[INDEX_TWO] - px[INDEX_ONE];
        }
        if (top_ >= other.bottom_ || bottom_ <= other.top_) {
            height = 0;
        } else {
            vector<int32_t> py = {top_, bottom_, other.top_, other.bottom_};
            sort(py.begin(), py.end());
            height = py[INDEX_TWO] - py[INDEX_ONE];
        }
    }

    bool Widget::HasAttr(string_view name) const
    {
        return attributes_.find(string(name)) != attributes_.end();
    }

    string Widget::GetAttr(string_view name, string_view defaultVal) const
    {
        auto find = attributes_.find(string(name));
        return find == attributes_.end() ? string(defaultVal) : find->second;
    }

    void Widget::SetAttr(string_view name, string_view value)
    {
        attributes_[string(name)] = value;
    }

    void Widget::SetHostTreeId(string_view tid)
    {
        hostTreeId_ = tid;
    }

    string Widget::GetHostTreeId() const
    {
        return hostTreeId_;
    }

    void Widget::SetBounds(int32_t cl, int32_t cr, int32_t ct, int32_t cb)
    {
        bounds_ = Rect{cl, cr, ct, cb};
    }

    string Widget::ToStr(bool ignoreBounds) const
    {
        stringstream os;
        os << "Widget<";
        for (auto &pair:attributes_) {
            if (ignoreBounds && pair.first == ATTR_NAMES[UiAttr::BOUNDS]) {
                continue;
            }
            os << pair.first << "='" << pair.second << "',";
        }
        os << ">";
        return os.str();
    }

    void Widget::DumpAttributes(map<string, string> &receiver) const
    {
        for (auto&[attr, value]:attributes_) {
            receiver[attr] = value;
        }
    }

    /**
     * Dom visitor to collect dom-widget attributes.
     * */
    class WidgetAttrCollector : public DomNodeVisitor {
    public:
        void Visit(string_view tag, string_view hierarchy, const map<string, string> &attributes) override
        {
            widgetAttrDict_.insert(make_pair(hierarchy, attributes));
            visitationTrace_.emplace_back(hierarchy);
        }

        void OnDomParseError(string_view message) override
        {
            domError_ = domError_ + "; " + string(message);
        }

        map<string, map<string, string>> widgetAttrDict_;
        vector<string> visitationTrace_;
        string domError_;
    };

    class WidgetHierarchyBuilder : public NodeHierarchyBuilder {
    public:
        string BuildForRootWidget() const override
        {
            return string(ROOT_HIERARCHY);
        }

        string Build(string_view parentWidgetHierarchy, uint32_t childIndex) const override
        {
            return string(parentWidgetHierarchy) + string(hierarchySeparator_) + to_string(childIndex);
        }

        static string GetParentWidgetHierarchy(string_view hierarchy)
        {
            if (hierarchy == ROOT_HIERARCHY) {
                // no parent for root widget
                return "";
            }

            auto findRoot = hierarchy.find(ROOT_HIERARCHY);
            if (findRoot != 0) {
                // invalid hierarchy string
                return "";
            }
            auto findLastSeparator = hierarchy.find_last_of(hierarchySeparator_);
            if (findLastSeparator <= 0 || findLastSeparator == string::npos) {
                return "";
            }
            return string(hierarchy).substr(0, findLastSeparator);
        }

        static string GetChildHierarchy(string_view hierarchy, uint32_t childIndex)
        {
            if (hierarchy.find(ROOT_HIERARCHY) != 0) {
                // invalid hierarchy string
                return "";
            }
            return string(hierarchy) + string(hierarchySeparator_) + to_string(childIndex);
        }

        static bool CheckIsDescendantHierarchyOfRoot(string_view hierarchy, string_view hierarchyRoot)
        {
            // child not hierarchy must startswith parent node hierarchy
            return hierarchy.find(hierarchyRoot) == 0;
        }

    private:
        static constexpr auto hierarchySeparator_ = ",";

    };

    static void SetWidgetBounds(Widget &widget, string_view boundsStr)
    {
        // set bounds
        int32_t val = -1;
        int32_t integers[4];
        int32_t index = 0;

        static constexpr int32_t FACTOR = 10;
        for (char ch : boundsStr) {
            if (ch >= '0' && ch <= '9') {
                val = max(val, 0); // ensure accumulation
                val = val * FACTOR + (ch - '0');
            } else if (val >= 0) {
                DCHECK(index < INDEX_FOUR, "Illegal boundStr, more than 4 integers");
                integers[index] = val;
                // after harvest, rest accumulation and increase ptrIdx
                index++;
                val = -1;
            }
        }

        DCHECK(index == INDEX_FOUR, "Illegal boundStr, less than 4 integers");
        widget.SetBounds(integers[INDEX_ZERO], integers[INDEX_TWO], integers[INDEX_ONE], integers[INDEX_THREE]);
    }

    static void SetWidgetAttributes(Widget &widget, const map<string, string> &attributes)
    {
        for (auto &item:attributes) {
            widget.SetAttr(item.first, item.second);
            if (item.first == ATTR_NAMES[UiAttr::BOUNDS]) {
                SetWidgetBounds(widget, item.second);
            }
        }
    }

    void WidgetTree::DfsTraverse(WidgetVisitor &visitor) const
    {
        auto root = GetRootWidget();
        if (root != nullptr) {
            DfsTraverseDescendants(visitor, *root);
        }
    }

    void WidgetTree::DfsTraverseFronts(WidgetVisitor &visitor, const Widget &pivot) const
    {
        DCHECK(widgetsConstructed_, "Illegal state, widgets not constructed yet");
        DCHECK(CheckIsMyNode(pivot), "Cannot traverse widget which dose not belongs to current tree");
        auto root = GetRootWidget();
        if (root == nullptr) {
            return;
        }
        auto pivotHierarchy = pivot.GetHierarchy();
        for (auto &hierarchy:widgetHierarchyIdDfsOrder_) {
            if (hierarchy == pivotHierarchy) {
                // hit the pivot, finish traversing
                break;
            }
            auto widget = widgetMap_.find(hierarchy);
            DCHECK(widget != widgetMap_.end(), "Failed to get widget by hierarchy id");
            visitor.Visit(widget->second);
        }
    }

    void WidgetTree::DfsTraverseRears(WidgetVisitor &visitor, const Widget &pivot) const
    {
        DCHECK(widgetsConstructed_, "Illegal state, widgets not constructed yet");
        DCHECK(CheckIsMyNode(pivot), "Cannot traverse widget which dose not belongs to current tree");
        auto root = GetRootWidget();
        if (root == nullptr) {
            return;
        }
        auto pivotHierarchy = pivot.GetHierarchy();
        bool traverseStarted = false;
        for (auto &hierarchy:widgetHierarchyIdDfsOrder_) {
            if (hierarchy == pivotHierarchy) {
                // skip self and start traverse from next one
                traverseStarted = true;
                continue;
            }
            if (!traverseStarted) {
                // skip front widgets
                continue;
            }
            auto widget = widgetMap_.find(hierarchy);
            DCHECK(widget != widgetMap_.end(), "Failed to get widget by hierarchy id");
            visitor.Visit(widget->second);
        }
    }

    void WidgetTree::DfsTraverseDescendants(WidgetVisitor &visitor, const Widget &root) const
    {
        DCHECK(widgetsConstructed_, "Illegal state, widgets not constructed yet");
        DCHECK(CheckIsMyNode(root), "Cannot traverse widget which dose not belongs to current tree");
        bool traverseStarted = false;
        auto rootHierarchy = root.GetHierarchy();
        for (auto &hierarchy:widgetHierarchyIdDfsOrder_) {
            if (!WidgetHierarchyBuilder::CheckIsDescendantHierarchyOfRoot(hierarchy, rootHierarchy)) {
                if (!traverseStarted) {
                    continue; // root node not found yet, skip visiting current widget and go ahead
                } else {
                    break; // descendant nodes are all visited, break
                }
            }
            traverseStarted = true;
            auto widget = widgetMap_.find(hierarchy);
            DCHECK(widget != widgetMap_.end(), "Failed to get widget by hierarchy id");
            visitor.Visit(widget->second);
        }
    }

    bool WidgetTree::ConstructFromDomText(string_view content, stringstream &errReceiver)
    {
        WidgetAttrCollector visitor;
        WidgetHierarchyBuilder hierarchyBuilder;
        DomTreeParser::DfsParse(content, visitor, hierarchyBuilder);
        if (!visitor.domError_.empty()) {
            errReceiver << visitor.domError_;
            return false;
        }

        // cached constructed widgets
        auto findRootAttrs = visitor.widgetAttrDict_.find(string(ROOT_HIERARCHY));
        DCHECK(findRootAttrs != visitor.widgetAttrDict_.end(), "Root widget not found in the dom tree");
        Widget root(ROOT_HIERARCHY);
        root.SetHostTreeId(this->identifier_);
        SetWidgetAttributes(root, findRootAttrs->second);
        widgetMap_.insert(make_pair(ROOT_HIERARCHY, move(root)));
        widgetHierarchyIdDfsOrder_.emplace_back(ROOT_HIERARCHY);

        for (auto it = visitor.visitationTrace_.begin() + 1; it != visitor.visitationTrace_.end(); it++) {
            auto findWidgetAttrs = visitor.widgetAttrDict_.find(*it);
            DCHECK(findWidgetAttrs != visitor.widgetAttrDict_.end(), "Visited widget missing attributes");
            auto parentHierarchy = WidgetHierarchyBuilder::GetParentWidgetHierarchy(*it);
            DCHECK(!parentHierarchy.empty(), "Non-root widget get parent hierarchy failed");
            // dfs order ensures parent widget is visited before its children widget
            auto findParent = widgetMap_.find(parentHierarchy);
            DCHECK(findParent != widgetMap_.end(), "Find parent widget in constructed widget failed");

            Widget widget(*it);
            widget.SetHostTreeId(this->identifier_);
            SetWidgetAttributes(widget, findWidgetAttrs->second);
            widgetMap_.insert(make_pair(*it, move(widget)));
            widgetHierarchyIdDfsOrder_.emplace_back(*it);
        }

        widgetsConstructed_ = true;
        return true;
    }

    const Widget *WidgetTree::GetRootWidget() const
    {
        return GetWidgetByHierarchy(ROOT_HIERARCHY);
    }

    const Widget *WidgetTree::GetParentWidget(const Widget &widget) const
    {
        DCHECK(CheckIsMyNode(widget), "Cannot handle widget which dose not belongs to current tree");
        if (widget.GetHierarchy() == ROOT_HIERARCHY) {
            return nullptr;
        }
        const auto parentHierarchy = WidgetHierarchyBuilder::GetParentWidgetHierarchy(widget.GetHierarchy());
        return GetWidgetByHierarchy(parentHierarchy);
    }

    const Widget *WidgetTree::GetChildWidget(const Widget &widget, uint32_t index) const
    {
        DCHECK(CheckIsMyNode(widget), "Cannot handle widget which dose not belongs to current tree");
        if (index < 0) {
            return nullptr;
        }
        auto childHierarchy = WidgetHierarchyBuilder::GetChildHierarchy(widget.GetHierarchy(), index);
        return GetWidgetByHierarchy(childHierarchy);
    }

    const Widget *WidgetTree::GetWidgetByHierarchy(string_view hierarchy) const
    {
        auto findWidget = widgetMap_.find(string(hierarchy));
        if (findWidget != widgetMap_.end()) {
            return &(findWidget->second);
        }
        return nullptr;
    }

    bool WidgetTree::IsRootWidgetHierarchy(string_view hierarchy)
    {
        return ROOT_HIERARCHY == hierarchy;
    }

    string WidgetTree::GenerateTreeId()
    {
        static uint32_t counter = 0;
        auto id = string("WidgetTree@") + to_string(counter);
        counter++;
        return id;
    }

    inline bool WidgetTree::CheckIsMyNode(const Widget &widget) const
    {
        return this->identifier_ == widget.GetHostTreeId();
    }
} // namespace uitest


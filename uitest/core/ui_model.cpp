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

#include <algorithm>
#include "common_defines.h"
#include "ui_model.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    static constexpr auto ROOT_HIERARCHY = "ROOT";

    string Rect::ToStr() const
    {
        stringstream os;
        os << "Rect<";
        os << "xLeft=" <<left_ << ",xRight=" << right_ << ",yTop=" << top_ << ",yButton=" << bottom_;
        os << ">";
        return os.str();
    }

    void Rect::ComputeOverlappingDimensions(const Rect &other, int32_t &width, int32_t& height) const
    {
        if (left_ >= other.right_ || right_ <= other.left_) {
            width = 0;
        } else {
            array<int32_t, INDEX_FOUR> px = {left_, right_, other.left_, other.right_};
            sort(px.begin(), px.end());
            width = px[INDEX_TWO] - px[INDEX_ONE];
        }
        if (top_ >= other.bottom_ || bottom_ <= other.top_) {
            height = 0;
        } else {
            array<int32_t, INDEX_FOUR> py = {top_, bottom_, other.top_, other.bottom_};
            sort(py.begin(), py.end());
            height = py[INDEX_TWO] - py[INDEX_ONE];
        }
    }

    bool Rect::ComputeIntersection(const Rect & other, Rect & result) const
    {
        if (left_ >= other.right_ || right_ <= other.left_) {
            return false;
        }
        if (top_ >= other.bottom_ || bottom_ <= other.top_) {
            return false;
        }
        array<int32_t, INDEX_FOUR> px = {left_, right_, other.left_, other.right_};
        array<int32_t, INDEX_FOUR> py = {top_, bottom_, other.top_, other.bottom_};
        sort(px.begin(), px.end());
        sort(py.begin(), py.end());
        result = {px[INDEX_ONE], px[INDEX_TWO], py[INDEX_ONE], py[INDEX_TWO]};
        return true;
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
        bounds_ = Rect {cl, cr, ct, cb};
    }

    string Widget::ToStr() const
    {
        stringstream os;
        os << "Widget{bounds=" << bounds_.ToStr() << ",";
        for (auto &pair:attributes_) {
            os << pair.first << "='" << pair.second << "',";
        }
        os << "}";
        return os.str();
    }

    void Widget::DumpAttributes(map<string, string> &receiver) const
    {
        for (auto&[attr, value]:attributes_) {
            receiver[attr] = value;
        }
    }

    class WidgetHierarchyBuilder {
    public:

        static string Build(string_view parentWidgetHierarchy, uint32_t childIndex)
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
            // child node hierarchy must startswith parent node hierarchy
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
        bool negative = false;
        static constexpr int32_t FACTOR = 10;
        for (char ch : boundsStr) {
            if (ch == '-') {
                DCHECK(val == -1); // should be a start of a number
                negative = true;
            } else if (ch >= '0' && ch <= '9') {
                val = max(val, 0); // ensure accumulation
                val = val * FACTOR + (int32_t)(ch - '0');
            } else if (val >= 0) {
                DCHECK(index < INDEX_FOUR);
                integers[index] = val * (negative ? -1 : 1);
                // after harvest, rest variables and increase ptrIdx
                index++;
                val = -1;
                negative = false;
            }
        }

        DCHECK(index == INDEX_FOUR);
        widget.SetBounds(integers[INDEX_ZERO], integers[INDEX_TWO], integers[INDEX_ONE], integers[INDEX_THREE]);
    }

    static void SetWidgetAttributes(Widget &widget, const map<string, string> &attributes)
    {
        for (auto &item:attributes) {
            if (item.first == ATTR_NAMES[UiAttr::BOUNDS]) {
                SetWidgetBounds(widget, item.second);
            } else {
                widget.SetAttr(item.first, item.second);
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
        DCHECK(widgetsConstructed_);
        DCHECK(CheckIsMyNode(pivot));
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
            DCHECK(widget != widgetMap_.end());
            visitor.Visit(widget->second);
        }
    }

    void WidgetTree::DfsTraverseRears(WidgetVisitor &visitor, const Widget &pivot) const
    {
        DCHECK(widgetsConstructed_);
        DCHECK(CheckIsMyNode(pivot));
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
            DCHECK(widget != widgetMap_.end());
            visitor.Visit(widget->second);
        }
    }

    void WidgetTree::DfsTraverseDescendants(WidgetVisitor &visitor, const Widget &root) const
    {
        DCHECK(widgetsConstructed_);
        DCHECK(CheckIsMyNode(root));
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
            DCHECK(widget != widgetMap_.end());
            visitor.Visit(widget->second);
        }
    }

    using NodeVisitor = function<void(string_view, map<string, string>&&)>;

    static void DfsVisitNode(const json &root, NodeVisitor visitor, string_view hierarchy)
    {
        DCHECK(visitor != nullptr);
        auto attributesData = root["attributes"];
        auto childrenData = root["children"];
        map<string, string> attributeDict;
        for (auto &item:attributesData.items()) {
            attributeDict[item.key()] = item.value();
        }
        visitor(hierarchy, move(attributeDict));
        const size_t childCount = childrenData.size();
        for (size_t idx = 0; idx < childCount; idx++) {
            auto &child = childrenData.at(idx);
            auto childHierarchy = WidgetHierarchyBuilder::Build(hierarchy, idx);
            DfsVisitNode(child, visitor, childHierarchy);
        }
    }

    void WidgetTree::ConstructFromDom(const nlohmann::json& dom, bool amendBounds)
    {
        DCHECK(!widgetsConstructed_);
        map<string, map<string, string>> widgetDict;
        vector<string> visitTrace;
        auto nodeVisitor = [&widgetDict, &visitTrace](string_view hierarchy, map<string, string>&& attrs) {
            visitTrace.emplace_back(hierarchy);
            widgetDict.insert(make_pair(hierarchy, attrs));
        };
        DfsVisitNode(dom, nodeVisitor, ROOT_HIERARCHY);
        for (auto& hierarchy : visitTrace) {
            auto findWidgetAttrs = widgetDict.find(hierarchy);
            DCHECK(findWidgetAttrs != widgetDict.end());
            Widget widget(hierarchy);
            widget.SetHostTreeId(this->identifier_);
            SetWidgetAttributes(widget, findWidgetAttrs->second);
            auto findParent = widgetMap_.find(WidgetHierarchyBuilder::GetParentWidgetHierarchy(hierarchy));
            const auto bounds = widget.GetBounds();
            auto visible = false;
            if (!amendBounds) {
                visible = true;
            } else if (hierarchy == ROOT_HIERARCHY) {
                visible = (bounds.right_ > bounds.left_) && (bounds.bottom_ > bounds.top_);
            } else if (findParent == widgetMap_.end()) {
                visible = false; // parent was discarded
            } else {
                // amend bounds, intersect with parent, compute visibility                
                auto parentBounds = findParent->second.GetBounds();
                auto newBounds = Rect {0, 0, 0, 0};
                if (bounds.ComputeIntersection(parentBounds, newBounds)) {
                    widget.SetBounds(newBounds.left_, newBounds.right_, newBounds.top_, newBounds.bottom_);
                    visible = (newBounds.right_ > newBounds.left_) && (newBounds.bottom_ > newBounds.top_);
                } else {
                    widget.SetBounds(0, 0, 0, 0);
                    visible = false;
                }
            }
            if (amendBounds && bounds.ToStr().compare(widget.GetBounds().ToStr()) != 0) {
                LOG_D("Amend bounds %{public}s from %{public}s", widget.ToStr().c_str(), bounds.ToStr().c_str());
            }
            if (visible) {
                widgetMap_.insert(make_pair(hierarchy, move(widget)));
                widgetHierarchyIdDfsOrder_.emplace_back(hierarchy);
            } else {
                LOG_D("Discard invisible node '%{public}s'and its descendants", widget.ToStr().c_str());
            }
        }
        widgetsConstructed_ = true;
    }

    const Widget *WidgetTree::GetRootWidget() const
    {
        return GetWidgetByHierarchy(ROOT_HIERARCHY);
    }

    const Widget *WidgetTree::GetParentWidget(const Widget &widget) const
    {
        DCHECK(CheckIsMyNode(widget));
        if (widget.GetHierarchy() == ROOT_HIERARCHY) {
            return nullptr;
        }
        const auto parentHierarchy = WidgetHierarchyBuilder::GetParentWidgetHierarchy(widget.GetHierarchy());
        return GetWidgetByHierarchy(parentHierarchy);
    }

    const Widget *WidgetTree::GetChildWidget(const Widget &widget, uint32_t index) const
    {
        DCHECK(CheckIsMyNode(widget));
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


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
#include "ui_model.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    static constexpr auto ROOT_HIERARCHY = "ROOT";

    static string Rect2JsonStr(const Rect &rect)
    {
        json data;
        data["left"] = rect.left_;
        data["top"] = rect.top_;
        data["right"] = rect.right_;
        data["bottom"] = rect.bottom_;
        return data.dump();
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

    void Widget::SetBounds(const Rect &bounds)
    {
        bounds_ = bounds;
        // save bounds attribute as structured data
        SetAttr(ATTR_NAMES[UiAttr::BOUNDS], Rect2JsonStr(bounds_));
    }

    string Widget::ToStr() const
    {
        stringstream os;
        os << "Widget{";
        for (auto &pair : attributes_) {
            os << pair.first << "='" << pair.second << "',";
        }
        os << "}";
        return os.str();
    }

    unique_ptr<Widget> Widget::Clone(string_view hostTreeId, string_view hierarchy) const
    {
        auto clone = make_unique<Widget>(hierarchy);
        clone->hostTreeId_ = hostTreeId;
        clone->attributes_ = this->attributes_;
        clone->bounds_ = this->bounds_;
        clone->SetAttr(ATTR_NAMES[UiAttr::HIERARCHY], hierarchy); // ensure hiararchy consisent
        return clone;
    }

    std::map<std::string, std::string> Widget::GetAttrMap() const
    {
        return attributes_;
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

        inline static bool CheckIsDescendantHierarchy(string_view hierarchy, string_view hierarchyRoot)
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
        auto rect = Rect(integers[INDEX_ZERO], integers[INDEX_TWO], integers[INDEX_ONE], integers[INDEX_THREE]);
        widget.SetBounds(rect);
    }

    static void SetWidgetAttributes(Widget &widget, const map<string, string> &attributes)
    {
        for (auto &item : attributes) {
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
        for (auto &hierarchy : widgetHierarchyIdDfsOrder_) {
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
        for (auto &hierarchy : widgetHierarchyIdDfsOrder_) {
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
        for (auto &hierarchy : widgetHierarchyIdDfsOrder_) {
            if (!WidgetHierarchyBuilder::CheckIsDescendantHierarchy(hierarchy, rootHierarchy)) {
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

    using NodeVisitor = function<void(string_view, map<string, string> &&)>;

    static void DfsVisitNode(const json &root, NodeVisitor visitor, string_view hierarchy)
    {
        DCHECK(visitor != nullptr);
        auto attributesData = root["attributes"];
        auto childrenData = root["children"];
        map<string, string> attributeDict;
        for (auto &item : attributesData.items()) {
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

    void WidgetTree::ConstructFromDom(const nlohmann::json &dom, bool amendBounds)
    {
        DCHECK(!widgetsConstructed_);
        map<string, map<string, string>> widgetDict;
        vector<string> visitTrace;
        auto nodeVisitor = [&widgetDict, &visitTrace](string_view hierarchy, map<string, string> &&attrs) {
            visitTrace.emplace_back(hierarchy);
            widgetDict.insert(make_pair(hierarchy, attrs));
        };
        DfsVisitNode(dom, nodeVisitor, ROOT_HIERARCHY);
        for (auto &hierarchy : visitTrace) {
            auto findWidgetAttrs = widgetDict.find(hierarchy);
            DCHECK(findWidgetAttrs != widgetDict.end());
            Widget widget(hierarchy);
            widget.SetHostTreeId(this->identifier_);
            SetWidgetAttributes(widget, findWidgetAttrs->second);
            auto findParent = widgetMap_.find(WidgetHierarchyBuilder::GetParentWidgetHierarchy(hierarchy));
            const auto bounds = widget.GetBounds();
            auto newBounds = Rect(0, 0, 0, 0);
            if (!amendBounds || hierarchy == ROOT_HIERARCHY) {
                newBounds = bounds;
            } else if (findParent == widgetMap_.end()) {
                newBounds = Rect(0, 0, 0, 0); // parent was discarded
            } else {
                // amend bounds, intersect with parent, compute visibility
                auto parentBounds = findParent->second.GetBounds();
                if (!RectAlgorithm::ComputeIntersection(bounds, parentBounds, newBounds)) {
                    newBounds = Rect(0, 0, 0, 0);
                }
            }
            if (!RectAlgorithm::CheckEqual(newBounds, bounds)) {
                widget.SetBounds(newBounds);
                LOG_D("Amend bounds %{public}s from %{public}s", widget.ToStr().c_str(), Rect2JsonStr(bounds).c_str());
            }
            if (!amendBounds || (newBounds.GetWidth() > 0 && newBounds.GetHeight() > 0)) {
                widgetMap_.insert(make_pair(hierarchy, move(widget)));
                widgetHierarchyIdDfsOrder_.emplace_back(hierarchy);
            } else {
                LOG_D("Discard invisible node '%{public}s'and its descendants", widget.ToStr().c_str());
            }
        }
        widgetsConstructed_ = true;
    }

    static void DfsMarshalWidget(const WidgetTree& tree, const Widget& root, nlohmann::json& dom,
        const std::map<string, size_t>& widgetChildCountMap)
    {
        auto attributesData = json();
        // "< UiAttr::HIERARCHY" : do not expose inner used attributes
        for (auto index = 0; index < UiAttr::HIERARCHY; index++) {
            const auto attr = ATTR_NAMES[index].data();
            attributesData[attr] = root.GetAttr(attr, "");
        }
        stringstream stream;
        auto rect = root.GetBounds();
        stream << "[" << rect.left_ << "," << rect.top_ << "]"
               << "[" << rect.right_ << "," << rect.bottom_ << "]";
        attributesData[ATTR_NAMES[UiAttr::BOUNDS].data()] = stream.str();

        auto childrenData = json::array();
        uint32_t childIndex = 0;
        uint32_t childCount = 0;
        uint32_t visitCount = 0;
        auto hierarchy = root.GetHierarchy();
        if (widgetChildCountMap.find(hierarchy) != widgetChildCountMap.end()) {
            childCount = widgetChildCountMap.find(hierarchy)->second;
        }
        while (visitCount < childCount) {
            auto child = tree.GetChildWidget(root, childIndex);
            childIndex++;
            if (child == nullptr) {
                continue;
            }
            auto childData = json();
            DfsMarshalWidget(tree, *child, childData, widgetChildCountMap);
            childrenData.emplace_back(childData);
            visitCount++;
        }

        dom["attributes"] = attributesData;
        dom["children"] = childrenData;
    }

    void WidgetTree::MarshalIntoDom(nlohmann::json& dom) const
    {
        DCHECK(widgetsConstructed_);
        auto root = GetRootWidget();
        std::map<string, size_t> widgetChildCountMap;
        for (auto &hierarchy : widgetHierarchyIdDfsOrder_) {
            if (hierarchy == ROOT_HIERARCHY) {
                continue;
            }
            auto parentHierarchy = WidgetHierarchyBuilder::GetParentWidgetHierarchy(hierarchy);
            if (widgetChildCountMap.find(parentHierarchy) == widgetChildCountMap.end()) {
                widgetChildCountMap[parentHierarchy] = 1;
            } else {
                widgetChildCountMap[parentHierarchy] = widgetChildCountMap[parentHierarchy] + 1;
            }
        }
        if (root != nullptr) {
            DfsMarshalWidget(*this, *root, dom, widgetChildCountMap);
        }
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

    /** WidgetVisitor used to visit and merge widgets of subtrees into dest root tree.*/
    class MergerVisitor : public WidgetVisitor {
    public:
        // handler function to perform merging widget, arg1: the revised bounds
        using MergerFunction = function<void(const Widget &, const Rect &)>;
        explicit MergerVisitor(MergerFunction collector) : collector_(collector) {};

        ~MergerVisitor() {}

        void PrepareToVisitSubTree(const WidgetTree &tree);

        void EndVisitingSubTree();

        void Visit(const Widget &widget) override;

        Rect GetMergedBounds() const
        {
            return mergedBounds_;
        }

    private:
        MergerFunction collector_;
        // the overlays of widget
        vector<Rect> overlays_;
        // the node hierarchy of first invisble parent
        string maxInvisibleParent_ = "NA";
        // the node hierarchy of first fully-visible parent
        string maxFullyParent_ = "NA";
        // the merged bounds
        Rect mergedBounds_ = {0, 0, 0, 0};
        // bounds of current visiting tree
        Rect visitingTreeBounds_ = {0, 0, 0, 0};
    };

    void MergerVisitor::PrepareToVisitSubTree(const WidgetTree &tree)
    {
        auto root = tree.GetRootWidget();
        DCHECK(root != nullptr);
        // collect max bounds
        auto rootBounds = root->GetBounds();
        mergedBounds_.left_ = min(mergedBounds_.left_, rootBounds.left_);
        mergedBounds_.top_ = min(mergedBounds_.top_, rootBounds.top_);
        mergedBounds_.right_ = max(mergedBounds_.right_, rootBounds.right_);
        mergedBounds_.bottom_ = max(mergedBounds_.bottom_, rootBounds.bottom_);
        // update visiting tree bounds
        visitingTreeBounds_ = rootBounds;
        // reset intermediate data
        maxInvisibleParent_ = "NA";
        maxFullyParent_ = "NA";
    }

    void MergerVisitor::EndVisitingSubTree()
    {
        // add overlays for later visited subtrees
        overlays_.push_back(visitingTreeBounds_);
    }

    void MergerVisitor::Visit(const Widget &widget)
    {
        if (collector_ == nullptr) {
            return;
        }
        const auto hierarchy = widget.GetHierarchy();
        const auto inInVisible = WidgetHierarchyBuilder::CheckIsDescendantHierarchy(hierarchy, maxInvisibleParent_);
        const auto inFully = WidgetHierarchyBuilder::CheckIsDescendantHierarchy(hierarchy, maxFullyParent_);
        if (inInVisible) {
            // parent invisible, skip
            return;
        }
        const auto bounds = widget.GetBounds();
        bool visible = true;
        auto visibleRegion = bounds;
        if (!inFully) {
            // parent not full-visible, need compute visible region
            visible = RectAlgorithm::ComputeMaxVisibleRegion(bounds, overlays_, visibleRegion);
        }
        if (!visible) {
            maxInvisibleParent_ = hierarchy; // update maxInvisibleParent
            return;
        }
        if (!inFully && RectAlgorithm::CheckEqual(bounds, visibleRegion)) {
            maxFullyParent_ = hierarchy; // update maxFullParent
        }
        // call collector with widget and revised bounds
        collector_(widget, visibleRegion);
    }

    void WidgetTree::MergeTrees(const vector<unique_ptr<WidgetTree>> &from, WidgetTree &to)
    {
        if (from.empty()) {
            return;
        }
        to.widgetsConstructed_ = true;
        auto virtualRoot = Widget(ROOT_HIERARCHY);
        virtualRoot.SetHostTreeId(to.identifier_);
        // insert virtual root node into tree
        to.widgetHierarchyIdDfsOrder_.emplace_back(ROOT_HIERARCHY);
        to.widgetMap_.insert(make_pair(ROOT_HIERARCHY, move(virtualRoot)));
        auto &vitualRootWidget = to.widgetMap_.begin()->second;
        // amend widget hierarchy with prefix to make it the descendant of the virtualRoot
        string hierarchyPrefix = "";
        constexpr auto offset = string_view(ROOT_HIERARCHY).length();
        // collect widget with revised hierarchy and bounds, merge it to dest tree
        auto merger = [&hierarchyPrefix, &tree = to](const Widget &widget, const Rect &bounds) {
            auto newHierarchy = string(hierarchyPrefix) + widget.GetHierarchy().substr(offset);
            auto newWidget = widget.Clone(tree.identifier_, newHierarchy);
            newWidget->SetBounds(bounds);
            tree.widgetMap_.insert(make_pair(newHierarchy, move(*newWidget)));
            tree.widgetHierarchyIdDfsOrder_.emplace_back(newHierarchy);
        };
        auto visitor = MergerVisitor(merger);
        size_t index = 0;
        for (auto &tree : from) {
            DCHECK(tree != nullptr);
            // update hierarchy prefix
            hierarchyPrefix = string(ROOT_HIERARCHY) + "," + to_string(index);
            // visit tree and forward visible widgets to collector
            visitor.PrepareToVisitSubTree(*tree);
            tree->DfsTraverse(visitor);
            visitor.EndVisitingSubTree();
            index++;
        }
        // amend bounds of the virtual root
        vitualRootWidget.SetBounds(visitor.GetMergedBounds());
    }
} // namespace OHOS::uitest

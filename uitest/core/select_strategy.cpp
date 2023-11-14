/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "select_strategy.h"
namespace OHOS::uitest {
    void SelectStrategy::RegisterAnchorMatch(const WidgetMatchModel &matchModel)
    {
        anchorMatch_.emplace_back(matchModel);
    }

    void SelectStrategy::RegisterMyselfMatch(const WidgetMatchModel &matchModel)
    {
        myselfMatch_.emplace_back(matchModel);
    }

    void SelectStrategy::SetWantMulti(bool isWantMulti)
    {
        wantMulti_ = isWantMulti;
    }

    void SelectStrategy::SetAndCalcSelectWindowRect(const Rect &windowBounds, const std::vector<Rect> &windowBoundsVec)
    {
        windowBounds_ = windowBounds;
        overplayWindowBoundsVec_ = windowBoundsVec;
    }

    SelectStrategy::~SelectStrategy()
    {
        anchorMatch_.clear();
        myselfMatch_.clear();
    }

    void SelectStrategy::CalcWidgetVisibleBounds(const Widget &widget,
                                                 const Rect &containerParentRect,
                                                 Rect &visibleRect)
    {
        // 1) 与自身window计算可见区域，计算重合区域
        Rect visibleInWindow{0, 0, 0, 0};
        if (!RectAlgorithm::ComputeIntersection(widget.GetBounds(), windowBounds_, visibleInWindow)) {
            LOG_I("Widget %{public}s bounds is %{public}s, without window bounds %{public}s, widget info is %{public}s",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), widget.GetBounds().Describe().data(),
                  windowBounds_.Describe().data(), widget.ToStr().data());
            visibleRect = Rect(0, 0, 0, 0);
            return;
        }
        // 2) 与父组件计算可见区域，取交集
        Rect visibleInParent{0, 0, 0, 0};
        if (!RectAlgorithm::ComputeIntersection(visibleInWindow, containerParentRect, visibleInParent)) {
            LOG_I("Widget %{public}s bounds is %{public}s, without parent bounds %{public}s, widget info is %{public}s",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), widget.GetBounds().Describe().data(),
                  containerParentRect.Describe().data(), widget.ToStr().data());
            visibleRect = Rect(0, 0, 0, 0);
            return;
        }
        if (!RectAlgorithm::CheckEqual(containerParentRect, windowBounds_) ||
            CONTAINER_TYPE.find(widget.GetAttr(UiAttr::TYPE)) != CONTAINER_TYPE.cend()) {
            if (widget.IsVisible() && (visibleInParent.GetHeight() == 0 || visibleInParent.GetWidth() == 0)) {
                LOG_I("Widget %{public}s height or widget is zero, but it is container, keep it visible",
                      widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                visibleRect = visibleInParent;
                return;
            }
        }
        if (overplayWindowBoundsVec_.size() == 0) {
            visibleRect = visibleInParent;
            return;
        }
        Rect visibleBounds{0, 0, 0, 0};
        auto visible = RectAlgorithm::ComputeMaxVisibleRegion(visibleInParent, overplayWindowBoundsVec_, visibleBounds);
        if (!visible) {
            LOG_I("widget %{public}s is hide by overplays, widget info is %{public}s",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), widget.ToStr().data());
            visibleRect = Rect{0, 0, 0, 0};
        } else {
            visibleRect = visibleBounds;
        }
    }

    std::string SelectStrategy::Describe() const
    {
        stringstream ss;
        ss << "{";
        ss << STRATEGY_NAMES[GetStrategyType()];
        if (!anchorMatch_.empty()) {
            ss << "; anchorMatcher=";
            for (auto &locator : anchorMatch_) {
                ss << "[" << locator.Describe() << "]";
            }
        }
        if (!myselfMatch_.empty()) {
            ss << "; myselfMatcher=";
            for (auto &locator : myselfMatch_) {
                ss << "[" << locator.Describe() << "]";
            }
        }
        ss << "}";
        return ss.str();
    }

    void SelectStrategy::RefreshWidgetBounds(const Rect &containerParentRect, Widget &widget)
    {
        Rect widgetVisibleBounds = Rect{0, 0, 0, 0};
        CalcWidgetVisibleBounds(widget, containerParentRect, widgetVisibleBounds);

        widget.SetBounds(widgetVisibleBounds);
        // [left,top][right, bottom]
        stringstream boundStream;
        boundStream << "[" << widgetVisibleBounds.left_ << "," << widgetVisibleBounds.top_ << "]["
                    << widgetVisibleBounds.right_ << "," << widgetVisibleBounds.bottom_ << "]";
        widget.SetAttr(UiAttr::BOUNDS, boundStream.str());

        if (widget.GetAttr(UiAttr::VISIBLE) == "false") {
            return;
        }
        if (widgetVisibleBounds.GetHeight() <= 0 && widgetVisibleBounds.GetWidth() <= 0) {
            widget.SetAttr(UiAttr::VISIBLE, "false");
        } else if (widgetVisibleBounds.GetHeight() <= 0 || widgetVisibleBounds.GetWidth() <= 0) {
            if (!RectAlgorithm::CheckEqual(containerParentRect, windowBounds_) ||
                CONTAINER_TYPE.find(widget.GetAttr(UiAttr::TYPE)) != CONTAINER_TYPE.cend()) {
                widget.SetAttr(UiAttr::VISIBLE, "true");
            } else {
                widget.SetAttr(UiAttr::VISIBLE, "false");
            }
        } else {
            widget.SetAttr(UiAttr::VISIBLE, "true");
        }
    }

    class AfterSelectorStrategy : public SelectStrategy {
    public:
        AfterSelectorStrategy() = default;
        void LocateNode(const Window &window,
                        ElementNodeIterator &elementNodeRef,
                        std::vector<Widget> &visitWidgets,
                        std::vector<int> &targetWidgets,
                        bool isRemoveInvisible = true) override
        {
            elementNodeRef.ClearDFSNext();
            SetAndCalcSelectWindowRect(window.bounds_, window.invisibleBoundsVec_);
            while (true) {
                Widget anchorWidget{"test"};
                if (!elementNodeRef.DFSNext(anchorWidget)) {
                    return;
                }
                anchorWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                Rect anchorParentInWindow = windowBounds_;
                elementNodeRef.GetParentContainerBounds(anchorParentInWindow);
                RefreshWidgetBounds(anchorParentInWindow, anchorWidget);
                // 不可见的节点，在迭代器中删除
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_I("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap(tempAnchorWidget.get().GetBounds());
                bool isAnchorMatch = true;
                for (const auto &anchorIt : anchorMatch_) {
                    isAnchorMatch = tempAnchorWidget.get().MatchAttr(anchorIt) && isAnchorMatch;
                    if (!isAnchorMatch) {
                        break;
                    }
                }
                if (!isAnchorMatch) {
                    continue;
                }
                LocateNodeAfterAnchor(window, elementNodeRef, visitWidgets, targetWidgets);
            }
        }

        void LocateNodeAfterAnchor(const Window &window, ElementNodeIterator &elementNodeRef,
                        std::vector<Widget> &visitWidgets,
                        std::vector<int> &targetWidgets)
        {
            while (true) {
                Widget myselfWidget{"myselfWidget"};
                if (!elementNodeRef.DFSNext(myselfWidget)) {
                    return;
                }
                myselfWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                Rect parentInWindow = windowBounds_;
                elementNodeRef.GetParentContainerBounds(parentInWindow);
                RefreshWidgetBounds(parentInWindow, myselfWidget);
                // 不可见的节点，在迭代器中删除
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_I("Widget %{public}s is invisible", myselfWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(myselfWidget));
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap(tempWidget.get().GetBounds());
                bool isMyselfMatch = true;
                for (const auto &myselfIt : myselfMatch_) {
                    isMyselfMatch = tempWidget.get().MatchAttr(myselfIt) && isMyselfMatch;
                    if (!isMyselfMatch) {
                        break;
                    }
                }
                if (!isMyselfMatch) {
                    continue;
                }
                targetWidgets.emplace_back(visitWidgets.size() - 1);
                if (!wantMulti_) {
                    return;
                }
            }
        }

        StrategyEnum GetStrategyType() const override
        {
            return StrategyEnum::IS_AFTER;
        }
        ~AfterSelectorStrategy() override = default;
    };

    class BeforeSelectorStrategy : public SelectStrategy {
    public:
        BeforeSelectorStrategy() = default;
        void LocateNode(const Window &window,
                        ElementNodeIterator &elementNodeRef,
                        std::vector<Widget> &visitWidgets,
                        std::vector<int> &targetWidgets,
                        bool isRemoveInvisible = true) override
        {
            elementNodeRef.ClearDFSNext();
            SetAndCalcSelectWindowRect(window.bounds_, window.invisibleBoundsVec_);
            while (true) {
                Widget anchorWidget{"anchorWidget"};
                if (!elementNodeRef.DFSNext(anchorWidget)) {
                    return;
                }
                anchorWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                Rect parentInWindow = windowBounds_;
                elementNodeRef.GetParentContainerBounds(parentInWindow);
                RefreshWidgetBounds(parentInWindow, anchorWidget);
                // 不可见的节点，在迭代器中删除
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_I("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap(tempAnchorWidget.get().GetBounds());
                bool isAnchorMatch = true;
                for (const auto &anchorIt : anchorMatch_) {
                    isAnchorMatch = tempAnchorWidget.get().MatchAttr(anchorIt) && isAnchorMatch;
                    if (!isAnchorMatch) {
                        break;
                    }
                }
                if (!isAnchorMatch) {
                    continue;
                }
                std::string endId = tempAnchorWidget.get().GetAttr(UiAttr::ACCESSIBILITY_ID);
                LocateNodeAfterBeforeAnchor(endId, visitWidgets, targetWidgets);
                if (!targetWidgets.empty() && !wantMulti_) {
                    return;
                }
            }
        }

        void LocateNodeAfterBeforeAnchor(std::string_view endId,
                                         std::vector<Widget> &visitWidgets,
                                         std::vector<int> &targetWidgets)
        {
            std::string currentId = "";
            while (currentId != endId) {
                for (int i = 0; i < visitWidgets.size(); ++i) {
                    bool isMyselfMatch = true;
                    currentId = visitWidgets[i].GetAttr(UiAttr::ACCESSIBILITY_ID);
                    for (const auto &myselfIt : myselfMatch_) {
                        isMyselfMatch = visitWidgets[i].MatchAttr(myselfIt) && isMyselfMatch;
                        if (!isMyselfMatch) {
                            break;
                        }
                    }
                    if (!isMyselfMatch) {
                        continue;
                    }
                    targetWidgets.emplace_back(i);
                    if (!wantMulti_) {
                        return;
                    }
                }
            }
        }

        StrategyEnum GetStrategyType() const override
        {
            return StrategyEnum::IS_BEFORE;
        }
        ~BeforeSelectorStrategy() override = default;
    };

    class WithInSelectorStrategy : public SelectStrategy {
    public:
        WithInSelectorStrategy() = default;
        void LocateNode(const Window &window,
                        ElementNodeIterator &elementNodeRef,
                        std::vector<Widget> &visitWidgets,
                        std::vector<int> &targetWidgets,
                        bool isRemoveInvisible = true) override
        {
            elementNodeRef.ClearDFSNext();
            SetAndCalcSelectWindowRect(window.bounds_, window.invisibleBoundsVec_);
            // withIn存在不需要遍历当前window所有节点信息场景，需要是否能找到节点判断
            while (true) {
                Widget anchorWidget{"withInWidget"};
                if (!elementNodeRef.DFSNext(anchorWidget)) {
                    return;
                }
                anchorWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                Rect anchorParentInWindow = windowBounds_;
                elementNodeRef.GetParentContainerBounds(anchorParentInWindow);
                RefreshWidgetBounds(anchorParentInWindow, anchorWidget);

                // 不可见的节点，在迭代器中删除
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_I("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap(tempAnchorWidget.get().GetBounds());
                bool isAnchorMatch = true;
                for (const auto &anchorIt : anchorMatch_) {
                    isAnchorMatch = tempAnchorWidget.get().MatchAttr(anchorIt) && isAnchorMatch;
                    if (!isAnchorMatch) {
                        break;
                    }
                }
                if (!isAnchorMatch) {
                    continue;
                }
                LocateNodeWithInAnchor(window, elementNodeRef, visitWidgets, targetWidgets);
                if (!targetWidgets.empty() && !wantMulti_) {
                    return;
                }
            }
        }

        void LocateNodeWithInAnchor(const Window &window,
                        ElementNodeIterator &elementNodeRef,
                        std::vector<Widget> &visitWidgets,
                        std::vector<int> &targetWidgets)
        {
            // 找到锚点之后，基于锚点重置当前Top信息
            elementNodeRef.RestoreNodeIndexByAnchor();
            while (true) {
                Widget myselfWidget{"myselfWidget"};
                if (!elementNodeRef.DFSNextWithInTarget(myselfWidget)) {
                    break;
                }
                myselfWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                Rect parentInWindow = windowBounds_;
                elementNodeRef.GetParentContainerBounds(parentInWindow);
                RefreshWidgetBounds(parentInWindow, myselfWidget);

                // 不可见的节点，在迭代器中删除
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_I("Widget %{public}s is invisible", myselfWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(myselfWidget));
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap(tempWidget.get().GetBounds());
                bool isMyselfMatch = true;
                for (const auto &myselfIt : myselfMatch_) {
                    isMyselfMatch = tempWidget.get().MatchAttr(myselfIt) && isMyselfMatch;
                    if (!isMyselfMatch) {
                        break;
                    }
                }
                if (!isMyselfMatch) {
                    continue;
                }
                targetWidgets.emplace_back(visitWidgets.size() - 1);
                if (!wantMulti_) {
                    return;
                }
            }
            elementNodeRef.ResetNodeIndexToAnchor();
        }

        StrategyEnum GetStrategyType() const override
        {
            return StrategyEnum::WITH_IN;
        }
        ~WithInSelectorStrategy() override = default;
    };

    class PlainSelectorStrategy : public SelectStrategy {
    public:
        PlainSelectorStrategy() = default;
        void LocateNode(const Window &window,
                        ElementNodeIterator &elementNodeRef,
                        std::vector<Widget> &visitWidgets,
                        std::vector<int> &targetWidgets,
                        bool isRemoveInvisible = true) override
        {
            elementNodeRef.ClearDFSNext();
            SetAndCalcSelectWindowRect(window.bounds_, window.invisibleBoundsVec_);
            // 普通查找不需要锚点
            while (true) {
                Widget myselfWidget{"myselfWidget"};
                if (!elementNodeRef.DFSNext(myselfWidget)) {
                    return;
                }
                myselfWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                if (isRemoveInvisible) {
                    Rect parentInWindow = windowBounds_;
                    elementNodeRef.GetParentContainerBounds(parentInWindow);
                    RefreshWidgetBounds(parentInWindow, myselfWidget);
                    // 不可见的节点，在迭代器中删除
                    if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                        elementNodeRef.RemoveInvisibleWidget();
                        continue;
                    }
                }
                visitWidgets.emplace_back(move(myselfWidget));
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap(tempWidget.get().GetBounds());
                bool isMyselfMatch = true;
                for (const auto &myselfIt : myselfMatch_) {
                    isMyselfMatch = tempWidget.get().MatchAttr(myselfIt) && isMyselfMatch;
                    if (!isMyselfMatch) {
                        break;
                    }
                }
                if (!isMyselfMatch) {
                    continue;
                }
                targetWidgets.emplace_back(visitWidgets.size() - 1);
                if (!wantMulti_) {
                    return;
                }
            }
            //
        }

        StrategyEnum GetStrategyType() const override
        {
            return StrategyEnum::PLAIN;
        }
        ~PlainSelectorStrategy() override = default;
    };

    class ComplexSelectorStrategy : public SelectStrategy {
    public:
        ComplexSelectorStrategy() = default;
        void LocateNode(const Window &window,
                        ElementNodeIterator &elementNodeRef,
                        std::vector<Widget> &visitWidgets,
                        std::vector<int> &targetWidgets,
                        bool isRemoveInvisible = true) override
        {
            elementNodeRef.ClearDFSNext();
            SetAndCalcSelectWindowRect(window.bounds_, window.invisibleBoundsVec_);
            std::vector<int> fakeTargetWidgets;
            // 普通查找不需要锚点
            while (true) {
                Widget myselfWidget{"myselfWidget"};
                if (!elementNodeRef.DFSNext(myselfWidget)) {
                    break;
                }
                myselfWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));

                Rect parentInWindow = windowBounds_;
                elementNodeRef.GetParentContainerBounds(parentInWindow);
                RefreshWidgetBounds(parentInWindow, myselfWidget);
                // 不可见的节点，在迭代器中删除
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    continue;
                }

                visitWidgets.emplace_back(move(myselfWidget));
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap(tempWidget.get().GetBounds());
                bool isMyselfMatch = true;
                for (const auto &myselfIt : myselfMatch_) {
                    isMyselfMatch = tempWidget.get().MatchAttr(myselfIt) && isMyselfMatch;
                    if (!isMyselfMatch) {
                        break;
                    }
                }
                if (!isMyselfMatch) {
                    continue;
                }
                fakeTargetWidgets.emplace_back(visitWidgets.size() - 1);
            }
            DoComplexSelect(visitWidgets, fakeTargetWidgets, targetWidgets);
        }

        void RegisterMultiAfterAnchor(const std::vector<WidgetMatchModel> &afterAnchor)
        {
            multiAfterAnchorMatcher.emplace_back(afterAnchor);
        }

        void RegisterMultiBeforeAnchor(const std::vector<WidgetMatchModel> &beforeAnchor)
        {
            multiBeforeAnchorMatcher.emplace_back(beforeAnchor);
        }

        void RegisterMultiWithInAnchor(const std::vector<WidgetMatchModel> &withInAnchor)
        {
            multiWithInAnchorMatcher.emplace_back(withInAnchor);
        }

        int CalCMaxAfterAnchorIndex(const std::vector<Widget> &visitWidgets) {
            // after从前向后遍历，找到最大的
            int startAfterIndex = -1;
            for (auto &afterLocator : multiAfterAnchorMatcher) {
                int startIndex = 0;
                for (; startIndex < visitWidgets.size(); ++startIndex) {
                    bool isFrontMatch = true;
                    for (auto &attrModel : afterLocator) {
                        isFrontMatch = isFrontMatch && visitWidgets[startIndex].MatchAttr(attrModel);
                        if (!isFrontMatch) {
                            break;
                        }
                    }
                    if (isFrontMatch) {
                        startAfterIndex = startAfterIndex > startIndex ? startAfterIndex : startIndex;
                        break;
                    }
                }
                // 没有找到，访问到了最后
                if (startIndex == visitWidgets.size()) {
                    startAfterIndex = visitWidgets.size();
                }
            }
            return startAfterIndex;
        }

        int CalCMinBeforeAnchorIndex(const std::vector<Widget> &visitWidgets) {
            int startBeforeIndex = visitWidgets.size();
            for (auto &beforeLocator : multiBeforeAnchorMatcher) {
                int beforeIndex = visitWidgets.size() - 1;
                for (; beforeIndex > 0; --beforeIndex) {
                    bool isRearMatch = true;
                    for (auto &attrModel : beforeLocator) {
                        isRearMatch = isRearMatch && visitWidgets[beforeIndex].MatchAttr(attrModel);
                        if (!isRearMatch) {
                            break;
                        }
                    }
                    if (isRearMatch) {
                        startBeforeIndex = startBeforeIndex > beforeIndex ? beforeIndex : startBeforeIndex;
                        break;
                    }
                }
                if (beforeIndex == 0) {
                    startBeforeIndex = 0;
                }
            }
            return startBeforeIndex;
        }

        bool CheckTargetIdByParentSelect(const std::vector<int>& parentIndexVec, const std::vector<Widget> &visitWidgets)
        {
            bool isAllLocatorMatch = true;
            for (auto &parentLocator : multiWithInAnchorMatcher) {
                bool hasValidParent = false;
                for (auto &parentIndex : parentIndexVec) {
                    bool isMatch = true;
                    for (auto &selfMatch : parentLocator) {
                        isMatch = isMatch && visitWidgets[parentIndex].MatchAttr(selfMatch);
                        if (!isMatch) {
                            break;
                        }
                    }
                    hasValidParent = hasValidParent || isMatch;
                    if (hasValidParent) {
                        break;
                    }
                }
                isAllLocatorMatch = isAllLocatorMatch && hasValidParent;
                if (!isAllLocatorMatch) {
                    break;
                }
            }
            return isAllLocatorMatch;
        }

        void LocateNodeWithInComplexSelect(std::vector<int> &filterParentValidId,
                                         const std::vector<Widget> &visitWidgets,
                                         const std::vector<int> &myselfTargets)
        {
            for (int targetId : myselfTargets) {
                std::string_view targetHie = visitWidgets[targetId].GetHierarchy();
                std::vector<int> parentIndexVec;
                for (int index = 0; index < visitWidgets.size(); ++index) {
                    std::string_view visitHie = visitWidgets[index].GetHierarchy();
                    if (targetHie.length() <= visitHie.length() || targetHie.find(visitHie) != 0) {
                        continue;
                    }
                    parentIndexVec.emplace_back(index);
                }
                bool isAllLocatorMatch = CheckTargetIdByParentSelect(parentIndexVec, visitWidgets);
                if (isAllLocatorMatch) {
                    filterParentValidId.emplace_back(targetId);
                }
            }
        }

        void DoComplexSelect(const std::vector<Widget> &visitWidgets,
                             std::vector<int> &fakeTargets,
                             std::vector<int> &myselfTargets)
        {
            // 找到front和before在visitWidget的边界index
            // after从前向后遍历，找到最大的
            int startAfterIndex = CalCMaxAfterAnchorIndex(visitWidgets);
            // before 从后向前遍历，找到最小的
            int startBeforeIndex = CalCMinBeforeAnchorIndex(visitWidgets);
            if (startBeforeIndex <= startAfterIndex) {
                return;
            }
            // 基于start，end对target进行过滤
            for (auto index : fakeTargets) {
                if (index > startAfterIndex && index < startBeforeIndex) {
                    myselfTargets.emplace_back(index);
                }
            }

            if (myselfTargets.empty()) {
                return;
            }
            if (multiWithInAnchorMatcher.empty()) {
                if (!wantMulti_) {
                    myselfTargets.erase(myselfTargets.begin() + 1, myselfTargets.end());
                }
                return;
            }
            // 基于parent进行过滤
            std::vector<int> filterParentValidId;
            LocateNodeWithInComplexSelect(filterParentValidId, visitWidgets, myselfTargets);
            if (filterParentValidId.empty()) {
                myselfTargets.clear();
                return;
            }
            if (wantMulti_) {
                myselfTargets = filterParentValidId;
            } else {
                myselfTargets.clear();
                myselfTargets.emplace_back(filterParentValidId.at(0));
            }
        }

        StrategyEnum GetStrategyType() const override
        {
            return StrategyEnum::COMPLEX;
        }
        ~ComplexSelectorStrategy() override = default;

    private:
        std::vector<std::vector<WidgetMatchModel>> multiAfterAnchorMatcher;
        std::vector<std::vector<WidgetMatchModel>> multiBeforeAnchorMatcher;
        std::vector<std::vector<WidgetMatchModel>> multiWithInAnchorMatcher;
    };

    std::unique_ptr<SelectStrategy> SelectStrategy::BuildSelectStrategy(StrategyBuildParam buildParam, bool isWantMulti)
    {
        if (buildParam.afterAnchorMatcherVec.empty() && buildParam.beforeAnchorMatcherVec.empty() &&
            buildParam.withInAnchorMatcherVec.empty()) {
            std::unique_ptr<SelectStrategy> selectStrategy = std::make_unique<PlainSelectorStrategy>();
            for (const auto &matchModel : buildParam.myselfMatcher) {
                selectStrategy->RegisterMyselfMatch(matchModel);
            }
            selectStrategy->SetWantMulti(isWantMulti);
            return selectStrategy;
        } else if (!buildParam.afterAnchorMatcherVec.empty() && (buildParam.afterAnchorMatcherVec.size() == 1) &&
                   buildParam.beforeAnchorMatcherVec.empty() && buildParam.withInAnchorMatcherVec.empty()) {
            std::unique_ptr<SelectStrategy> selectStrategy = std::make_unique<AfterSelectorStrategy>();
            for (const auto &matchModel : buildParam.myselfMatcher) {
                selectStrategy->RegisterMyselfMatch(matchModel);
            }
            for (const auto &anchorModel : buildParam.afterAnchorMatcherVec.at(0)) {
                selectStrategy->RegisterAnchorMatch(anchorModel);
            }
            selectStrategy->SetWantMulti(isWantMulti);
            return selectStrategy;
        } else if (buildParam.afterAnchorMatcherVec.empty() && !buildParam.beforeAnchorMatcherVec.empty() &&
                   (buildParam.beforeAnchorMatcherVec.size() == 1) && buildParam.withInAnchorMatcherVec.empty()) {
            std::unique_ptr<SelectStrategy> selectStrategy = std::make_unique<BeforeSelectorStrategy>();
            for (const auto &matchModel : buildParam.myselfMatcher) {
                selectStrategy->RegisterMyselfMatch(matchModel);
            }
            for (const auto &anchorModel : buildParam.beforeAnchorMatcherVec.at(0)) {
                selectStrategy->RegisterAnchorMatch(anchorModel);
            }
            selectStrategy->SetWantMulti(isWantMulti);
            return selectStrategy;
        } else if (buildParam.afterAnchorMatcherVec.empty() && buildParam.beforeAnchorMatcherVec.empty() &&
                   !buildParam.withInAnchorMatcherVec.empty() && (buildParam.withInAnchorMatcherVec.size() == 1)) {
            std::unique_ptr<SelectStrategy> selectStrategy = std::make_unique<WithInSelectorStrategy>();
            for (const auto &matchModel : buildParam.myselfMatcher) {
                selectStrategy->RegisterMyselfMatch(matchModel);
            }
            for (const auto &anchorModel : buildParam.withInAnchorMatcherVec.at(0)) {
                selectStrategy->RegisterAnchorMatch(anchorModel);
            }
            selectStrategy->SetWantMulti(isWantMulti);
            return selectStrategy;
        } else {
            std::unique_ptr<ComplexSelectorStrategy> selectStrategy = std::make_unique<ComplexSelectorStrategy>();
            for (const auto &matchModel : buildParam.myselfMatcher) {
                selectStrategy->RegisterMyselfMatch(matchModel);
            }
            for (const auto &afterWidgetAnchor : buildParam.afterAnchorMatcherVec) {
                selectStrategy->RegisterMultiAfterAnchor(afterWidgetAnchor);
            }
            for (const auto &beforeWidgetAnchor : buildParam.beforeAnchorMatcherVec) {
                selectStrategy->RegisterMultiBeforeAnchor(beforeWidgetAnchor);
            }
            for (const auto &withInWidgetAnchor : buildParam.withInAnchorMatcherVec) {
                selectStrategy->RegisterMultiWithInAnchor(withInWidgetAnchor);
            }
            selectStrategy->SetWantMulti(isWantMulti);
            return selectStrategy;
        }
    }
} // namespace OHOS::uitest
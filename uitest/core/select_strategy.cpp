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
        // calc bounds with its window
        Rect visibleInWindow{0, 0, 0, 0};
        if (!RectAlgorithm::ComputeIntersection(widget.GetBounds(), windowBounds_, visibleInWindow)) {
            LOG_D("Widget %{public}s bounds is %{public}s, without window bounds %{public}s, widget info is %{public}s",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), widget.GetBounds().Describe().data(),
                  windowBounds_.Describe().data(), widget.ToStr().data());
            visibleRect = Rect(0, 0, 0, 0);
            return;
        }
        // calc bounds with its container parent
        Rect visibleInParent{0, 0, 0, 0};
        if (!RectAlgorithm::ComputeIntersection(visibleInWindow, containerParentRect, visibleInParent)) {
            LOG_D("Widget %{public}s bounds is %{public}s, without parent bounds %{public}s, widget info is %{public}s",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), widget.GetBounds().Describe().data(),
                  containerParentRect.Describe().data(), widget.ToStr().data());
            visibleRect = Rect(0, 0, 0, 0);
            return;
        }
        if (!RectAlgorithm::CheckEqual(containerParentRect, windowBounds_) ||
            CONTAINER_TYPE.find(widget.GetAttr(UiAttr::TYPE)) != CONTAINER_TYPE.cend()) {
            if (widget.IsVisible() && (visibleInParent.GetHeight() == 0 || visibleInParent.GetWidth() == 0)) {
                LOG_D("Widget %{public}s height or widget is zero, but it is container, keep it visible",
                      widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                visibleRect = visibleInParent;
                return;
            }
        }
        if (overplayWindowBoundsVec_.size() == 0) {
            visibleRect = visibleInParent;
            return;
        }
        // calc bounds with overplay windows
        Rect visibleBounds{0, 0, 0, 0};
        auto visible = RectAlgorithm::ComputeMaxVisibleRegion(visibleInParent, overplayWindowBoundsVec_, visibleBounds);
        if (!visible) {
            LOG_D("widget %{public}s is hide by overplays, widget info is %{public}s",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), widget.ToStr().data());
            visibleRect = Rect{0, 0, 0, 0};
        } else {
            visibleRect = visibleBounds;
        }
    }

    std::string SelectStrategy::Describe() const
    {
        std::string strategy = "PLAIN";
        switch (GetStrategyType()) {
            case StrategyEnum::PLAIN:
                strategy = "PLAIN";
                break;
            case StrategyEnum::WITH_IN:
                strategy = "WITH_IN";
                break;
            case StrategyEnum::IS_AFTER:
                strategy = "IS_AFTER";
                break;
            case StrategyEnum::IS_BEFORE:
                strategy = "IS_BEFORE";
                break;
            case StrategyEnum::COMPLEX:
                strategy = "COMPLEX";
                break;
            default:
                LOG_I("Error StrategyType, use plain");
                strategy = "PLAIN";
                break;
        }
        stringstream ss;
        ss << "{";
        ss << strategy;
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
        Rect oriBounds = widget.GetBounds();
        // parent rect is 0, but it has child, keep visible from access not refresh bound
        if ((oriBounds.GetHeight() == 0 || oriBounds.GetWidth() == 0) &&
            (oriBounds.left_ >= 0 && oriBounds.top_ >= 0)) {
            LOG_D("Widget %{public}s height or widget is zero, rect is %{public}s, keep it visible",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), oriBounds.Describe().data());
            return;
        }
        Rect widgetVisibleBounds = Rect{0, 0, 0, 0};
        CalcWidgetVisibleBounds(widget, containerParentRect, widgetVisibleBounds);

        widget.SetBounds(widgetVisibleBounds);
        if (widgetVisibleBounds.GetHeight() <= 0 && widgetVisibleBounds.GetWidth() <= 0) {
            widget.SetAttr(UiAttr::VISIBLE, "false");
            return;
        }
        if (widget.GetAttr(UiAttr::VISIBLE) == "false") {
            return;
        }
        if (widgetVisibleBounds.GetHeight() <= 0 || widgetVisibleBounds.GetWidth() <= 0) {
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
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap();
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

        void LocateNodeAfterAnchor(const Window &window,
                                   ElementNodeIterator &elementNodeRef,
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
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_D("Widget %{public}s is invisible", myselfWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(myselfWidget));
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap();
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
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap();
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
                LocateNodeAfterBeforeAnchor(visitWidgets, targetWidgets);
                if (!targetWidgets.empty() && !wantMulti_) {
                    return;
                }
            }
        }

        void LocateNodeAfterBeforeAnchor(std::vector<Widget> &visitWidgets,
                                         std::vector<int> &targetWidgets)
        {
            int index = 0;
            if (targetWidgets.size() > 0) {
                index = targetWidgets[targetWidgets.size() - 1] + 1;
            }
            for (; index < visitWidgets.size() - 1; ++index) {
                bool isMyselfMatch = true;
                for (const auto &myselfIt : myselfMatch_) {
                    isMyselfMatch = visitWidgets[index].MatchAttr(myselfIt) && isMyselfMatch;
                }
                if (!isMyselfMatch) {
                    continue;
                }
                targetWidgets.emplace_back(index);
                if (!wantMulti_) {
                    return;
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
            while (true) {
                Widget anchorWidget{"withInWidget"};
                if (!elementNodeRef.DFSNext(anchorWidget)) {
                    return;
                }
                anchorWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                Rect anchorParentInWindow = windowBounds_;
                elementNodeRef.GetParentContainerBounds(anchorParentInWindow);
                RefreshWidgetBounds(anchorParentInWindow, anchorWidget);
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap();
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
            // restore current index and set top to anchor index
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

                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    LOG_D("Widget %{public}s is invisible", myselfWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(myselfWidget));
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap();
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
                    if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                        elementNodeRef.RemoveInvisibleWidget();
                        continue;
                    }
                }
                visitWidgets.emplace_back(move(myselfWidget));
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap();
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
            while (true) {
                Widget myselfWidget{"myselfWidget"};
                if (!elementNodeRef.DFSNext(myselfWidget)) {
                    break;
                }
                myselfWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));

                Rect parentInWindow = windowBounds_;
                elementNodeRef.GetParentContainerBounds(parentInWindow);
                RefreshWidgetBounds(parentInWindow, myselfWidget);
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    elementNodeRef.RemoveInvisibleWidget();
                    continue;
                }

                visitWidgets.emplace_back(move(myselfWidget));
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
                elementNodeRef.CheckAndUpdateContainerRectMap();
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

        int CalcMaxAfterAnchorIndex(const std::vector<Widget> &visitWidgets)
        {
            int startAfterIndex = -1;
            for (auto &afterLocator : multiAfterAnchorMatcher) {
                int startIndex = 0;
                for (; startIndex < visitWidgets.size(); ++startIndex) {
                    bool isFrontMatch = true;
                    for (auto &attrModel : afterLocator) {
                        isFrontMatch = isFrontMatch && visitWidgets[startIndex].MatchAttr(attrModel);
                    }
                    if (isFrontMatch) {
                        startAfterIndex = startAfterIndex > startIndex ? startAfterIndex : startIndex;
                        break;
                    }
                }
                if (startIndex == visitWidgets.size()) {
                    startAfterIndex = visitWidgets.size();
                }
            }
            return startAfterIndex;
        }

        int CalcMinBeforeAnchorIndex(const std::vector<Widget> &visitWidgets)
        {
            int startBeforeIndex = visitWidgets.size();
            for (auto &beforeLocator : multiBeforeAnchorMatcher) {
                int beforeIndex = visitWidgets.size() - 1;
                for (; beforeIndex > 0; --beforeIndex) {
                    bool isRearMatch = true;
                    for (auto &attrModel : beforeLocator) {
                        isRearMatch = isRearMatch && visitWidgets[beforeIndex].MatchAttr(attrModel);
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

        bool CheckTargetIdByParentSelect(const std::vector<int> &parentIndexVec,
                                         const std::vector<Widget> &visitWidgets)
        {
            bool isAllLocatorMatch = true;
            for (auto &parentLocator : multiWithInAnchorMatcher) {
                bool hasValidParent = false;
                for (auto &parentIndex : parentIndexVec) {
                    bool isMatch = true;
                    for (auto &selfMatch : parentLocator) {
                        isMatch = isMatch && visitWidgets[parentIndex].MatchAttr(selfMatch);
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
                const std::string &targetHie = visitWidgets[targetId].GetHierarchy();
                std::vector<int> parentIndexVec;
                for (int index = 0; index < visitWidgets.size(); ++index) {
                    const std::string &visitHie = visitWidgets[index].GetHierarchy();
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
            int startAfterIndex = CalcMaxAfterAnchorIndex(visitWidgets);
            int startBeforeIndex = CalcMinBeforeAnchorIndex(visitWidgets);
            if (startBeforeIndex <= startAfterIndex) {
                return;
            }
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

    static std::unique_ptr<SelectStrategy> BuildComplexStrategy(const StrategyBuildParam &buildParam, bool isWantMulti)
    {
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

    static std::unique_ptr<SelectStrategy> BuildWithInStrategy(const StrategyBuildParam &buildParam, bool isWantMulti)
    {
        std::unique_ptr<SelectStrategy> selectStrategy = std::make_unique<WithInSelectorStrategy>();
        for (const auto &matchModel : buildParam.myselfMatcher) {
            selectStrategy->RegisterMyselfMatch(matchModel);
        }
        for (const auto &anchorModel : buildParam.withInAnchorMatcherVec.at(0)) {
            selectStrategy->RegisterAnchorMatch(anchorModel);
        }
        selectStrategy->SetWantMulti(isWantMulti);
        return selectStrategy;
    }

    static std::unique_ptr<SelectStrategy> BuildBeforeStrategy(const StrategyBuildParam &buildParam, bool isWantMulti)
    {
        std::unique_ptr<SelectStrategy> selectStrategy = std::make_unique<BeforeSelectorStrategy>();
        for (const auto &matchModel : buildParam.myselfMatcher) {
            selectStrategy->RegisterMyselfMatch(matchModel);
        }
        for (const auto &anchorModel : buildParam.beforeAnchorMatcherVec.at(0)) {
            selectStrategy->RegisterAnchorMatch(anchorModel);
        }
        selectStrategy->SetWantMulti(isWantMulti);
        return selectStrategy;
    }

    static std::unique_ptr<SelectStrategy> BuildAfterStrategy(const StrategyBuildParam &buildParam, bool isWantMulti)
    {
        std::unique_ptr<SelectStrategy> selectStrategy = std::make_unique<AfterSelectorStrategy>();
        for (const auto &matchModel : buildParam.myselfMatcher) {
            selectStrategy->RegisterMyselfMatch(matchModel);
        }
        for (const auto &anchorModel : buildParam.afterAnchorMatcherVec.at(0)) {
            selectStrategy->RegisterAnchorMatch(anchorModel);
        }
        selectStrategy->SetWantMulti(isWantMulti);
        return selectStrategy;
    }

    std::unique_ptr<SelectStrategy> SelectStrategy::BuildSelectStrategy(const StrategyBuildParam &buildParam,
                                                                        bool isWantMulti)
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
            return BuildAfterStrategy(buildParam, isWantMulti);
        } else if (buildParam.afterAnchorMatcherVec.empty() && !buildParam.beforeAnchorMatcherVec.empty() &&
                   (buildParam.beforeAnchorMatcherVec.size() == 1) && buildParam.withInAnchorMatcherVec.empty()) {
            return BuildBeforeStrategy(buildParam, isWantMulti);
        } else if (buildParam.afterAnchorMatcherVec.empty() && buildParam.beforeAnchorMatcherVec.empty() &&
                   !buildParam.withInAnchorMatcherVec.empty() && (buildParam.withInAnchorMatcherVec.size() == 1)) {
            return BuildWithInStrategy(buildParam, isWantMulti);
        } else {
            return BuildComplexStrategy(buildParam, isWantMulti);
        }
    }
} // namespace OHOS::uitest
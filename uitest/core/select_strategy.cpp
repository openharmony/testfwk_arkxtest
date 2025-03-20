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
    constexpr int32_t MAX_TRAVEL_TIMES = 20000;
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

    void SelectStrategy::CalcWidgetVisibleBounds(Widget &widget)
    {
        const auto noneZone = Rect{0, 0, 0, 0};
        // calc bounds with its window
        Rect visibleRect{0, 0, 0, 0};
        if (!RectAlgorithm::ComputeIntersection(widget.GetBounds(), windowBounds_, visibleRect)) {
            LOG_D("Widget %{public}s bounds is %{public}s, without window bounds %{public}s, widget info is %{public}s",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), widget.GetBounds().Describe().data(),
                  windowBounds_.Describe().data(), widget.ToStr().data());
            widget.SetBounds(noneZone);
            return;
        }
        widget.SetBounds(visibleRect);

        // calc bounds with its cliper
        auto hier = widget.GetAttr(UiAttr::HIERARCHY);
        while (WidgetHierarchyBuilder::GetParentWidgetHierarchy(hier) != "") {
            std::string parentHie = WidgetHierarchyBuilder::GetParentWidgetHierarchy(hier);
            auto find = clipers_.find(parentHie);
            if (find != clipers_.end()) {
                if (!RectAlgorithm::ComputeIntersection(widget.GetBounds(), find->second, visibleRect)) {
                    widget.SetBounds(noneZone);
                    return;
                }
                widget.SetBounds(visibleRect);
                break;
            }
            hier = parentHie;
        }

        // calc bounds with overplay windows
        if (overplayWindowBoundsVec_.size() == 0) {
            return;
        }
        if (!RectAlgorithm::ComputeMaxVisibleRegion(widget.GetBounds(), overplayWindowBoundsVec_, visibleRect)) {
            LOG_D("widget %{public}s is hide by overplays, widget info is %{public}s",
                  widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data(), widget.ToStr().data());
            widget.SetBounds(noneZone);
        }
        widget.SetBounds(visibleRect);
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

    void SelectStrategy::RefreshWidgetBounds(Widget &widget)
    {
        CalcWidgetVisibleBounds(widget);
        auto widgetVisibleBounds = widget.GetBounds();
        if (widgetVisibleBounds.GetHeight() <= 0 || widgetVisibleBounds.GetWidth() <= 0) {
            widget.SetAttr(UiAttr::VISIBLE, "false");
        } else {
            widget.SetAttr(UiAttr::VISIBLE, "true");
        }
        if (widget.GetAttr(UiAttr::CLIP) == "true") {
            clipers_.insert(make_pair(widget.GetAttr(UiAttr::HIERARCHY), widget.GetBounds()));
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
                if (!elementNodeRef.DFSNext(anchorWidget, window.id_)) {
                    return;
                }
                anchorWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    elementNodeRef.RemoveInvisibleWidget();
                    continue;
                }
                RefreshWidgetBounds(anchorWidget);
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                if (visitWidgets.size() > MAX_TRAVEL_TIMES) {
                    LOG_E("ElementInfos obtained from AAMS is abnormal, traversal node failed");
                    visitWidgets.clear();
                    return;
                }
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
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
                if (!elementNodeRef.DFSNext(myselfWidget, window.id_)) {
                    return;
                }
                myselfWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                RefreshWidgetBounds(myselfWidget);
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", myselfWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(myselfWidget));
                if (visitWidgets.size() > MAX_TRAVEL_TIMES) {
                    LOG_E("ElementInfos obtained from AAMS is abnormal, traversal node failed");
                    visitWidgets.clear();
                    return;
                }
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
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
                if (!elementNodeRef.DFSNext(anchorWidget, window.id_)) {
                    return;
                }
                anchorWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    elementNodeRef.RemoveInvisibleWidget();
                    continue;
                }
                RefreshWidgetBounds(anchorWidget);
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                if (visitWidgets.size() > MAX_TRAVEL_TIMES) {
                    LOG_E("ElementInfos obtained from AAMS is abnormal, traversal node failed");
                    visitWidgets.clear();
                    return;
                }
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
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
            size_t index = 0;
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
                if (!elementNodeRef.DFSNext(anchorWidget, window.id_)) {
                    return;
                }
                anchorWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    elementNodeRef.RemoveInvisibleWidget();
                    continue;
                }
                RefreshWidgetBounds(anchorWidget);
                if (anchorWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", anchorWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(anchorWidget));
                if (visitWidgets.size() > MAX_TRAVEL_TIMES) {
                    LOG_E("ElementInfos obtained from AAMS is abnormal, traversal node failed");
                    visitWidgets.clear();
                    return;
                }
                std::reference_wrapper<Widget const> tempAnchorWidget = visitWidgets.back();
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
                RefreshWidgetBounds(myselfWidget);

                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", myselfWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    continue;
                }
                visitWidgets.emplace_back(move(myselfWidget));
                if (visitWidgets.size() > MAX_TRAVEL_TIMES) {
                    LOG_E("ElementInfos obtained from AAMS is abnormal, traversal node failed");
                    visitWidgets.clear();
                    return;
                }
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
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
                if (!elementNodeRef.DFSNext(myselfWidget, window.id_)) {
                    return;
                }
                myselfWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                if (isRemoveInvisible) {
                    if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                        LOG_D("Widget %{public}s is invisible", myselfWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                        elementNodeRef.RemoveInvisibleWidget();
                        continue;
                    }
                    RefreshWidgetBounds(myselfWidget);
                }
                visitWidgets.emplace_back(move(myselfWidget));
                if (visitWidgets.size() > MAX_TRAVEL_TIMES) {
                    LOG_E("ElementInfos obtained from AAMS is abnormal, traversal node failed");
                    visitWidgets.clear();
                    return;
                }
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    continue;
                }
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
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
                if (!elementNodeRef.DFSNext(myselfWidget, window.id_)) {
                    break;
                }
                myselfWidget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(window.id_));
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    LOG_D("Widget %{public}s is invisible", myselfWidget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
                    elementNodeRef.RemoveInvisibleWidget();
                    continue;
                }
                RefreshWidgetBounds(myselfWidget);
                if (myselfWidget.GetAttr(UiAttr::VISIBLE) == "false") {
                    continue;
                }
                visitWidgets.emplace_back(move(myselfWidget));
                if (visitWidgets.size() > MAX_TRAVEL_TIMES) {
                    LOG_E("ElementInfos obtained from AAMS is abnormal, traversal node failed");
                    visitWidgets.clear();
                    return;
                }
                std::reference_wrapper<Widget const> tempWidget = visitWidgets.back();
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
            int32_t startAfterIndex = -1;
            for (auto &afterLocator : multiAfterAnchorMatcher) {
                int32_t startIndex = 0;
                for (; static_cast<size_t>(startIndex) < visitWidgets.size(); ++startIndex) {
                    bool isFrontMatch = true;
                    for (auto &attrModel : afterLocator) {
                        isFrontMatch = isFrontMatch && visitWidgets[startIndex].MatchAttr(attrModel);
                    }
                    if (isFrontMatch) {
                        startAfterIndex = startAfterIndex > startIndex ? startAfterIndex : startIndex;
                        break;
                    }
                }
                if (static_cast<size_t>(startIndex) == visitWidgets.size()) {
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
                for (size_t index = 0; index < visitWidgets.size(); ++index) {
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
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

#ifndef SELECT_STRATEGY_H
#define SELECT_STRATEGY_H

#include "ui_model.h"
#include "element_node_iterator.h"

namespace OHOS::uitest {
    enum class StrategyEnum {
        PLAIN,
        WITH_IN,
        IS_AFTER,
        IS_BEFORE,
        COMPLEX,
    };

    struct StrategyBuildParam {
        std::vector<WidgetMatchModel> myselfMatcher;
        std::vector<std::vector<WidgetMatchModel>> afterAnchorMatcherVec;
        std::vector<std::vector<WidgetMatchModel>> beforeAnchorMatcherVec;
        std::vector<std::vector<WidgetMatchModel>> withInAnchorMatcherVec;
    };

    class SelectStrategy {
    public:
        SelectStrategy() = default;
        static unique_ptr<SelectStrategy> BuildSelectStrategy(const StrategyBuildParam &buildParam, bool isWantMulti);
        virtual void SetAndCalcSelectWindowRect(const Rect &windowBounds, const std::vector<Rect> &windowBoundsVec);
        virtual std::string Describe() const;
        virtual void RegisterAnchorMatch(const WidgetMatchModel &matchModel);
        virtual void RegisterMyselfMatch(const WidgetMatchModel &matchModel);
        virtual void SetWantMulti(bool isWantMulti);
        virtual StrategyEnum GetStrategyType() const = 0;
        virtual void LocateNode(const Window &window, ElementNodeIterator &elementNodeRef,
            vector<Widget> &visitWidgets, vector<int> &targetWidgets, const DumpOption &option) = 0;
        virtual ~SelectStrategy();

    protected:
        virtual void RefreshWidgetBounds(Widget &widget);
        virtual void CalcWidgetVisibleBounds(Widget &widget);
        std::vector<WidgetMatchModel> anchorMatch_;
        std::vector<WidgetMatchModel> myselfMatch_;
        bool wantMulti_ = false;
        Rect windowBounds_{0, 0, 0, 0};
        std::vector<Rect> overplayWindowBoundsVec_;
        std::map<string, Rect> clipers_;
    };
} // namespace OHOS::uitest

#endif
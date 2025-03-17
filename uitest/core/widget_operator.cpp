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

#include "widget_operator.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;
    
    static constexpr float SCROLL_MOVE_FACTOR = 0.7;

    static bool IsScrolledToBorder(int oriDis,
                                   const std::vector<unique_ptr<Widget>> &allWidgets,
                                   const unique_ptr<Widget> &anchorLeafWidget,
                                   bool vertical)
    {
        if (oriDis < 0) {
            return false;
        }
        size_t index = 0;
        for (; index < allWidgets.size(); ++index) {
            if (allWidgets.at(index)->GetAttr(UiAttr::ACCESSIBILITY_ID) ==
                anchorLeafWidget->GetAttr(UiAttr::ACCESSIBILITY_ID)) {
                if (vertical) {
                    return std::abs(allWidgets.at(index)->GetBounds().top_ - anchorLeafWidget->GetBounds().top_) <
                        oriDis * SCROLL_MOVE_FACTOR;
                } else {
                    return std::abs(allWidgets.at(index)->GetBounds().left_ - anchorLeafWidget->GetBounds().left_) <
                        oriDis * SCROLL_MOVE_FACTOR;
                }
            }
        }
        return false;
    }

    static void ConstructNoFilterInWidgetSelector(WidgetSelector &scrollSelector,
                                                  const std::string &hostApp,
                                                  const std::string &hashCode)
    {
        WidgetSelector parentStrategy;
        WidgetMatchModel anchorModel2{UiAttr::HASHCODE, hashCode, ValueMatchPattern::EQ};
        parentStrategy.AddMatcher(anchorModel2);
        scrollSelector.AddAppLocator(hostApp);
        auto error = ApiCallErr(NO_ERROR);
        scrollSelector.AddParentLocator(parentStrategy, error);
        scrollSelector.SetWantMulti(true);
    }

    static int CalcFirstLeafWithInScroll(const std::vector<unique_ptr<Widget>>& widgetsInScroll)
    {
        std::string parentHie = "ROOT";
        int index = -1;
        for (auto &tempWid : widgetsInScroll) {
            const std::string &curHie = tempWid->GetHierarchy();
            if (curHie.find(parentHie) == std::string::npos) {
                break;
            }
            parentHie = curHie;
            ++index;
        }
        return index;
    }

    static WidgetSelector ConstructScrollFindSelector(const WidgetSelector &selector, const string &hashcode,
        const string &appName, ApiCallErr &error)
    {
        WidgetSelector newSelector = selector;
        WidgetMatchModel anchorModel{UiAttr::HASHCODE, hashcode, ValueMatchPattern::EQ};
        WidgetSelector parentSelector{};
        parentSelector.AddMatcher(anchorModel);
        newSelector.AddParentLocator(parentSelector, error);
        newSelector.AddAppLocator(appName);
        newSelector.SetWantMulti(false);
        return newSelector;
    }

    WidgetOperator::WidgetOperator(UiDriver &driver, const Widget &widget, const UiOpArgs &options)
        : driver_(driver), widget_(widget), options_(options)
    {
    }

    void WidgetOperator::GenericClick(TouchOp op, ApiCallErr &error) const
    {
        DCHECK(op >= TouchOp::CLICK && op <= TouchOp::DOUBLE_CLICK_P);
        auto retrieved = driver_.RetrieveWidget(widget_, error, true);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        const auto center = Point(retrieved->GetBounds().GetCenterX(), retrieved->GetBounds().GetCenterY(),
            retrieved->GetDisplayId());
        auto touch = OHOS::uitest::GenericClick(op, center);
        driver_.PerformTouch(touch, options_, error);
    }

    void WidgetOperator::ScrollToEnd(bool toTop, ApiCallErr &error) const
    {
        auto retrieved = driver_.RetrieveWidget(widget_, error, true);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        int turnDis = -1;
        std::unique_ptr<Widget> lastTopLeafWidget = nullptr;
        std::unique_ptr<Widget> lastBottomLeafWidget = nullptr;
        while (true) {
            auto hostApp = driver_.GetHostApp(widget_);
            WidgetSelector selector{};
            ConstructNoFilterInWidgetSelector(selector, hostApp, widget_.GetAttr(UiAttr::HASHCODE));
            std::vector<unique_ptr<Widget>> widgetsInScroll;
            driver_.FindWidgets(selector, widgetsInScroll, error, true);
            if (error.code_ != NO_ERROR) {
                LOG_E("There is error when ScrollToEnd, msg is %{public}s", error.message_.c_str());
                return;
            }
            if (widgetsInScroll.empty()) {
                LOG_I("There is no child when ScrollToEnd");
                return;
            }
            if (toTop && IsScrolledToBorder(turnDis, widgetsInScroll, lastTopLeafWidget, true)) {
                return;
            }
            if (!toTop && IsScrolledToBorder(turnDis, widgetsInScroll, lastBottomLeafWidget, true)) {
                return;
            }
            if (toTop) {
                int index = CalcFirstLeafWithInScroll(widgetsInScroll);
                if (index < 0) {
                    LOG_E("There is error when Find Widget's fist leaf");
                    return;
                }
                lastTopLeafWidget = std::move(widgetsInScroll.at(index));
                lastBottomLeafWidget = std::move(widgetsInScroll.at(widgetsInScroll.size() - 1));
            } else {
                lastBottomLeafWidget = std::move(widgetsInScroll.at(widgetsInScroll.size() - 1));
            }
            TurnPage(toTop, turnDis, true, error);
        }
    }

    void WidgetOperator::DragIntoWidget(const Widget &another, ApiCallErr &error) const
    {
        auto widgetFrom = driver_.RetrieveWidget(widget_, error);
        if (widgetFrom == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto boundsFrom = widgetFrom->GetBounds();
        auto widgetTo = driver_.RetrieveWidget(another, error, false);
        if (widgetTo == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto boundsTo = widgetTo->GetBounds();
        auto centerFrom = Point(boundsFrom.GetCenterX(), boundsFrom.GetCenterY(), widgetFrom->GetDisplayId());
        auto centerTo = Point(boundsTo.GetCenterX(), boundsTo.GetCenterY(), widgetTo->GetDisplayId());
        auto touch = GenericSwipe(TouchOp::DRAG, centerFrom, centerTo);
        driver_.PerformTouch(touch, options_, error);
    }

    void WidgetOperator::PinchWidget(float_t scale, ApiCallErr &error) const
    {
        auto retrieved = driver_.RetrieveWidget(widget_, error);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto rectBound = retrieved->GetBounds();
        if (scale < 0) {
            error = ApiCallErr(ERR_INVALID_INPUT, "Please input the correct scale");
            return;
        }
        auto touch = GenericPinch(rectBound, scale);
        driver_.PerformTouch(touch, options_, error);
    }

    void WidgetOperator::InputText(string_view text, ApiCallErr &error) const
    {
        auto retrieved = driver_.RetrieveWidget(widget_, error);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return;
        }
        auto origText = retrieved->GetAttr(UiAttr::TEXT);
        if (origText.empty() && text.empty()) {
            return;
        }
        static constexpr uint32_t focusTimeMs = 500;
        static constexpr uint32_t typeCharTimeMs = 50;
        vector<KeyEvent> events;
        if (!origText.empty()) {
            for (size_t index = 0; index < origText.size(); index++) {
                events.emplace_back(KeyEvent{ActionStage::DOWN, KEYCODE_DPAD_RIGHT, typeCharTimeMs});
                events.emplace_back(KeyEvent{ActionStage::UP, KEYCODE_DPAD_RIGHT, 0});
                events.emplace_back(KeyEvent{ActionStage::DOWN, KEYCODE_DEL, typeCharTimeMs});
                events.emplace_back(KeyEvent{ActionStage::UP, KEYCODE_DEL, 0});
            }
        }
        const auto center = Point(retrieved->GetBounds().GetCenterX(), retrieved->GetBounds().GetCenterY(),
            retrieved->GetDisplayId());
        auto touch = OHOS::uitest::GenericClick(TouchOp::CLICK, center);
        driver_.PerformTouch(touch, options_, error);
        driver_.DelayMs(focusTimeMs); // short delay to ensure focus gaining
        auto keyActionForDelete = KeysForwarder(events);
        driver_.TriggerKey(keyActionForDelete, options_, error);
        driver_.DelayMs(focusTimeMs);
        driver_.InputText(text, error);
    }

    unique_ptr<Widget> WidgetOperator::ScrollFindWidget(const WidgetSelector &selector,
                                                        bool vertical, ApiCallErr &error) const
    {
        auto retrieved = driver_.RetrieveWidget(widget_, error);
        if (retrieved == nullptr || error.code_ != NO_ERROR) {
            return nullptr;
        }
        bool scrollingUp = true;
        int turnDis = -1;
        std::unique_ptr<Widget> lastTopLeafWidget = nullptr;
        std::unique_ptr<Widget> lastBottomLeafWidget = nullptr;
        auto hostApp = driver_.GetHostApp(widget_);
        auto newSelector = ConstructScrollFindSelector(selector, widget_.GetAttr(UiAttr::HASHCODE), hostApp, error);
        while (true) {
            std::vector<unique_ptr<Widget>> targetsInScroll;
            driver_.FindWidgets(newSelector, targetsInScroll, error, true);
            if (!targetsInScroll.empty()) {
                return std::move(targetsInScroll.at(0));
            }
            WidgetSelector scrollSelector;
            ConstructNoFilterInWidgetSelector(scrollSelector, hostApp, widget_.GetAttr(UiAttr::HASHCODE));
            std::vector<unique_ptr<Widget>> widgetsInScroll;
            driver_.FindWidgets(scrollSelector, widgetsInScroll, error, false);
            if (error.code_ != NO_ERROR) {
                LOG_E("There is error when Find Widget's subwidget, msg is %{public}s", error.message_.c_str());
                return nullptr;
            }
            if (widgetsInScroll.empty()) {
                LOG_I("There is no child when Find Widget's subwidget");
                return nullptr;
            }
            if (scrollingUp && IsScrolledToBorder(turnDis, widgetsInScroll, lastTopLeafWidget, vertical)) {
                scrollingUp = false;
            } else if (IsScrolledToBorder(turnDis, widgetsInScroll, lastBottomLeafWidget, vertical)) {
                LOG_W("Scroll search widget failed: %{public}s", selector.Describe().data());
                return nullptr;
            }
            if (scrollingUp) {
                int index = CalcFirstLeafWithInScroll(widgetsInScroll);
                if (index < 0) {
                    LOG_E("There is error when Find Widget's fist leaf");
                    return nullptr;
                }
                lastTopLeafWidget = std::move(widgetsInScroll.at(index));
                lastBottomLeafWidget = std::move(widgetsInScroll.at(widgetsInScroll.size() - 1));
            } else {
                lastBottomLeafWidget = std::move(widgetsInScroll.at(widgetsInScroll.size() - 1));
            }
            TurnPage(scrollingUp, turnDis, vertical, error);
        }
    }

    bool WidgetOperator::CheckDeadZone(bool vertical, ApiCallErr &error)
    {
        auto bounds = widget_.GetBounds();
        int maxDeadZone;
        if (vertical) {
            maxDeadZone = (bounds.bottom_ - bounds.top_) / TWO;
        } else {
            maxDeadZone = (bounds.right_ - bounds.left_) / TWO;
        }
        if (options_.scrollWidgetDeadZone_ >= maxDeadZone) {
            error = ApiCallErr(ERR_INVALID_INPUT, "The offset is too large and exceeds the widget size.");
            return false;
        } else {
            return true;
        }
    }
    void WidgetOperator::TurnPage(bool toTop, int &oriDistance, bool vertical, ApiCallErr &error) const
    {
        auto bounds = widget_.GetBounds();
        Point topPoint, bottomPoint;
        auto screenSize = driver_.GetDisplaySize(error, widget_.GetDisplayId());
        auto gestureZone = (vertical) ? screenSize.py_ / 20 : screenSize.px_ / 20;
        topPoint = vertical ? Point(bounds.GetCenterX(), bounds.top_) : Point(bounds.left_, bounds.GetCenterY());
        bottomPoint = vertical ? Point(bounds.GetCenterX(), bounds.bottom_) :
            Point(bounds.right_, bounds.GetCenterY());
        if (vertical) {
            if (options_.scrollWidgetDeadZone_ > 0) {
                topPoint.py_ += options_.scrollWidgetDeadZone_;
                bottomPoint.py_ -= options_.scrollWidgetDeadZone_;
            }
            if (abs(screenSize.py_ - bottomPoint.py_) <= gestureZone) {
                bottomPoint.py_ = screenSize.py_ - gestureZone;
            }
            if (vertical && topPoint.py_ <= gestureZone) {
                topPoint.py_ = gestureZone;
            }
        } else {
            if (options_.scrollWidgetDeadZone_ > 0) {
                topPoint.px_ += options_.scrollWidgetDeadZone_;
                bottomPoint.px_ -= options_.scrollWidgetDeadZone_;
            }
            if (abs(screenSize.px_ - bottomPoint.px_) <= gestureZone) {
                bottomPoint.px_ = screenSize.px_ - gestureZone;
            }
            if (topPoint.px_ <= gestureZone) {
                topPoint.px_ = gestureZone;
            }
        }
        topPoint.displayId_ = widget_.GetDisplayId();
        bottomPoint.displayId_ = widget_.GetDisplayId();
        auto touch = (toTop) ? GenericSwipe(TouchOp::SWIPE, topPoint, bottomPoint)
                             : GenericSwipe(TouchOp::SWIPE, bottomPoint, topPoint);
        driver_.PerformTouch(touch, options_, error);
        oriDistance = (vertical) ? std::abs(topPoint.py_ - bottomPoint.py_) : std::abs(topPoint.px_ - bottomPoint.px_);
        if (vertical && toTop) {
            LOG_I("turn page vertical from %{public}d to %{public}d", topPoint.py_, bottomPoint.py_);
        } else if (vertical) {
            LOG_I("turn page vertical from %{public}d to %{public}d", bottomPoint.py_, topPoint.py_);
        } else if (toTop) {
            LOG_I("turn page horizontal from %{public}d to %{public}d", topPoint.px_, bottomPoint.px_);
        } else {
            LOG_I("turn page horizontal from %{public}d to %{public}d", bottomPoint.px_, topPoint.px_);
        }
    }
} // namespace OHOS::uitest

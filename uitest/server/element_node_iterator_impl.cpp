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

#include "element_node_iterator_impl.h"
namespace OHOS::uitest {
    using namespace OHOS::Accessibility;

    ElementNodeIteratorImpl::ElementNodeIteratorImpl(
        const std::vector<AccessibilityElementInfo> &elements)
    {
        elementInfoLists_ = std::move(elements);
        currentIndex_ = -1;
        topIndex_ = 0;
        lastCurrentIndex_ = -1;
    }

    ElementNodeIteratorImpl::ElementNodeIteratorImpl()
    {
        currentIndex_ = -1;
        topIndex_ = 0;
        lastCurrentIndex_ = -1;
    }

    ElementNodeIteratorImpl::~ElementNodeIteratorImpl()
    {
        elementInfoLists_.clear();
        elementToParentIndexMap_.clear();
        elementIndexToHierarch_.clear();
        visitAndVisibleIndexSet_.clear();
    }

    bool ElementNodeIteratorImpl::DFSNextWithInTarget(Widget &widget)
    {
        if (currentIndex_ == topIndex_) {
            int count = elementInfoLists_[currentIndex_].GetChildCount();
            if (count <= 0) {
                return false;
            }
            for (size_t i = currentIndex_ + 1; i < elementInfoLists_.size(); ++i) {
                if (elementInfoLists_[i].GetAccessibilityId() != elementInfoLists_[currentIndex_].GetChildId(0)) {
                    continue;
                }
                elementToParentIndexMap_.emplace(i, currentIndex_);
                std::string parentHierarchy = elementIndexToHierarch_.at(currentIndex_);
                currentIndex_ = i;
                visitAndVisibleIndexSet_.insert(currentIndex_);
                WrapperElement(widget);
                widget.SetHierarchy(WidgetHierarchyBuilder::Build(parentHierarchy, 0));
                elementIndexToHierarch_.emplace(currentIndex_, widget.GetHierarchy());
                return true;
            }
        }
        return VisitNodeByChildAndBrother(widget);
    }

    bool ElementNodeIteratorImpl::DFSNext(Widget &widget, uint32_t windowId)
    {
        if (IsVisitFinish()) {
            return false;
        }

        if (currentIndex_ == -1) {
            currentIndex_ = 0;
            elementToParentIndexMap_.emplace(currentIndex_, -1);
            visitAndVisibleIndexSet_.insert(currentIndex_);
            WrapperElement(widget);
            widget.SetHierarchy(ROOT_HIERARCHY + to_string(windowId));
            elementIndexToHierarch_.emplace(currentIndex_, widget.GetHierarchy());
            return true;
        }
        return VisitNodeByChildAndBrother(widget);
    }

    bool ElementNodeIteratorImpl::IsVisitFinish() const
    {
        return elementInfoLists_.size() == elementToParentIndexMap_.size();
    }

    void ElementNodeIteratorImpl::RestoreNodeIndexByAnchor()
    {
        topIndex_ = currentIndex_;
        lastCurrentIndex_ = currentIndex_;
        LOG_D("select in widget, store current index is %{public}d", currentIndex_);
    }

    void ElementNodeIteratorImpl::ResetNodeIndexToAnchor()
    {
        topIndex_ = 0;
    }

    void ElementNodeIteratorImpl::ClearDFSNext()
    {
        elementToParentIndexMap_.clear();
        elementIndexToHierarch_.clear();
        visitAndVisibleIndexSet_.clear();
        currentIndex_ = -1;
        topIndex_ = 0;
    }

    void ElementNodeIteratorImpl::RemoveInvisibleWidget()
    {
        visitAndVisibleIndexSet_.erase(currentIndex_);
    }

    std::string ElementNodeIteratorImpl::GenerateNodeHashCode(const AccessibilityElementInfo &element)
    {
        int32_t winId = element.GetWindowId();
        int64_t accessId = element.GetAccessibilityId();
        std::stringstream hashcodeStr;
        hashcodeStr << winId;
        hashcodeStr << ":";
        hashcodeStr << accessId;
        return hashcodeStr.str();
    }

    void ElementNodeIteratorImpl::WrapperElement(Widget &widget)
    {
        AccessibilityElementInfo element = elementInfoLists_[currentIndex_];
        WrapperNodeAttrToVec(widget, element);
    }

    bool ElementNodeIteratorImpl::VisitNodeByChildAndBrother(Widget &widget)
    {
        int childCount = elementInfoLists_[currentIndex_].GetChildCount();
        if (childCount > 0) {
            if (VisitChildren(widget)) {
                return true;
            }
        }

        if (elementToParentIndexMap_.find(currentIndex_) == elementToParentIndexMap_.cend()) {
            LOG_D("This node has no parent: %{public}s",
                  std::to_string(elementInfoLists_[currentIndex_].GetAccessibilityId()).data());
            return false;
        }
        int parentIndex = elementToParentIndexMap_.at(currentIndex_);
        int tempChildIndex = currentIndex_;
        if (elementToParentIndexMap_.find(topIndex_) == elementToParentIndexMap_.cend()) {
            LOG_D("This topIndex_ has no parent: %{public}d", topIndex_);
            return false;
        }
        while (parentIndex != elementToParentIndexMap_.at(topIndex_)) {
            if (VisitBrother(widget, parentIndex, tempChildIndex)) {
                return true;
            }
            if (elementToParentIndexMap_.find(parentIndex) == elementToParentIndexMap_.cend()) {
                LOG_D("This node has no parent: %{public}s",
                      std::to_string(elementInfoLists_[parentIndex].GetAccessibilityId()).data());
                return false;
            }
            tempChildIndex = parentIndex;
            parentIndex = elementToParentIndexMap_.at(parentIndex);
        }
        return false;
    }

    bool ElementNodeIteratorImpl::VisitChildren(Widget& widget)
    {
        if (visitAndVisibleIndexSet_.find(currentIndex_) == visitAndVisibleIndexSet_.cend()) {
            LOG_D("node %{public}s is invisible not find its children",
                  std::to_string(elementInfoLists_[currentIndex_].GetAccessibilityId()).data());
            return false;
        } else {
            for (size_t i = currentIndex_ + 1; i < elementInfoLists_.size(); ++i) {
                if (elementInfoLists_[i].GetAccessibilityId() != elementInfoLists_[currentIndex_].GetChildId(0)) {
                    continue;
                }
                elementToParentIndexMap_.emplace(i, currentIndex_);
                std::string parentHierarchy = elementIndexToHierarch_.at(currentIndex_);
                currentIndex_ = i;
                visitAndVisibleIndexSet_.insert(currentIndex_);
                WrapperElement(widget);
                widget.SetHierarchy(WidgetHierarchyBuilder::Build(parentHierarchy, 0));
                elementIndexToHierarch_.emplace(currentIndex_, widget.GetHierarchy());
                return true;
            }
            return false;
        }
    }

    bool ElementNodeIteratorImpl::VisitBrother(Widget &widget, int parentIndex, int tempChildIndex)
    {
        AccessibilityElementInfo &parentModel = elementInfoLists_[parentIndex];
        int parentChildCount = parentModel.GetChildCount();
        for (int i = 0; i < parentChildCount; ++i) {
            if ((parentModel.GetChildId(i) == elementInfoLists_[tempChildIndex].GetAccessibilityId()) &&
                (i < parentChildCount - 1)) {
                if (parentModel.GetChildId(i + 1) == elementInfoLists_[tempChildIndex + 1].GetAccessibilityId()) {
                    elementToParentIndexMap_.emplace(tempChildIndex + 1, parentIndex);
                    std::string parentHierarchy = elementIndexToHierarch_.at(parentIndex);
                    currentIndex_ = tempChildIndex + 1;
                    visitAndVisibleIndexSet_.insert(currentIndex_);
                    WrapperElement(widget);
                    widget.SetHierarchy(WidgetHierarchyBuilder::Build(parentHierarchy, i + 1));
                    elementIndexToHierarch_.emplace(currentIndex_, widget.GetHierarchy());
                    return true;
                } else {
                    LOG_E("Node error, except: %{public}s, actual is %{public}s",
                          std::to_string(parentModel.GetChildId(i + 1)).data(),
                          std::to_string(elementInfoLists_[tempChildIndex + 1].GetAccessibilityId()).data());
                }
            }
        }
        return false;
    }

    void ElementNodeIteratorImpl::WrapperNodeAttrToVec(Widget &widget, const AccessibilityElementInfo &element)
    {
        Accessibility::Rect nodeOriginRect = elementInfoLists_[currentIndex_].GetRectInScreen();
        Rect visibleRect{nodeOriginRect.GetLeftTopXScreenPostion(), nodeOriginRect.GetRightBottomXScreenPostion(),
                         nodeOriginRect.GetLeftTopYScreenPostion(), nodeOriginRect.GetRightBottomYScreenPostion()};
        widget.SetBounds(visibleRect);
        widget.SetAttr(UiAttr::ACCESSIBILITY_ID, std::to_string(element.GetAccessibilityId()));
        widget.SetAttr(UiAttr::ID, element.GetInspectorKey());
        widget.SetAttr(UiAttr::TEXT, element.GetContent());
        widget.SetAttr(UiAttr::KEY, element.GetInspectorKey());
        widget.SetAttr(UiAttr::TYPE, element.GetComponentType());
        widget.SetAttr(UiAttr::DESCRIPTION, element.GetDescriptionInfo());
        if (element.GetComponentType() == "rootdecortag" || element.GetInspectorKey() == "ContainerModalTitleRow") {
            widget.SetAttr(UiAttr::TYPE, "DecorBar");
        }
        widget.SetAttr(UiAttr::ENABLED, element.IsEnabled() ? "true" : "false");
        widget.SetAttr(UiAttr::FOCUSED, element.IsFocused() ? "true" : "false");
        widget.SetAttr(UiAttr::SELECTED, element.IsSelected() ? "true" : "false");
        widget.SetAttr(UiAttr::CLICKABLE, "false");
        widget.SetAttr(UiAttr::LONG_CLICKABLE, "false");
        widget.SetAttr(UiAttr::SCROLLABLE, element.IsScrollable() ? "true" : "false");
        widget.SetAttr(UiAttr::CHECKABLE, element.IsCheckable() ? "true" : "false");
        widget.SetAttr(UiAttr::CHECKED, element.IsChecked() ? "true" : "false");
        widget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(element.GetWindowId()));
        widget.SetAttr(UiAttr::ZINDEX, std::to_string(element.GetZIndex()));
        widget.SetAttr(UiAttr::OPACITY, std::to_string(element.GetOpacity()));
        widget.SetAttr(UiAttr::BACKGROUNDCOLOR, element.GetBackgroundColor());
        widget.SetAttr(UiAttr::BACKGROUNDIMAGE, element.GetBackgroundImage());
        widget.SetAttr(UiAttr::BLUR, element.GetBlur());
        widget.SetAttr(UiAttr::HITTESTBEHAVIOR, element.GetHitTestBehavior());
        widget.SetAttr(UiAttr::CLIP, element.GetClip() ? "true" : "false");
        stringstream boundStream;
        boundStream << "[" << visibleRect.left_ << "," << visibleRect.top_ << "][" << visibleRect.right_ << ","
                    << visibleRect.bottom_ << "]";
        widget.SetAttr(UiAttr::ORIGBOUNDS, boundStream.str());
        widget.SetAttr(UiAttr::HASHCODE, ElementNodeIteratorImpl::GenerateNodeHashCode(element));
        if (!element.IsVisible()) {
            LOG_D("widget %{public}s is not visible", widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
        }
        widget.SetAttr(UiAttr::VISIBLE, element.IsVisible() ? "true" : "false");
        const auto app = element.GetBundleName();
        widget.SetAttr(UiAttr::BUNDLENAME, app);
        widget.SetAttr(UiAttr::HINT, element.GetHint());
        WrapperNodeActionAttrToVec(widget, element);
    }

    void ElementNodeIteratorImpl::WrapperNodeActionAttrToVec(Widget &widget, const AccessibilityElementInfo &element)
    {
        auto actions = element.GetActionList();
        for (auto &action : actions) {
            switch (action.GetActionType()) {
                case ACCESSIBILITY_ACTION_CLICK:
                    widget.SetAttr(UiAttr::CLICKABLE, "true");
                    break;
                case ACCESSIBILITY_ACTION_LONG_CLICK:
                    widget.SetAttr(UiAttr::LONG_CLICKABLE, "true");
                    break;
                default:
                    break;
            }
        }
    }
} // namespace OHOS::uitest
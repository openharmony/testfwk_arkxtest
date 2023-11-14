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
        // 搜索起点为自己
        if (currentIndex_ == topIndex_) {
            int count = elementInfoLists_[currentIndex_].GetChildCount();
            if (count <= 0) {
                return false;
            }
            for (int i = currentIndex_ + 1; i < elementInfoLists_.size(); ++i) {
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

    bool ElementNodeIteratorImpl::DFSNext(Widget &widget)
    {
        if (IsVisitFinish()) {
            return false;
        }

        // 首次查找
        if (currentIndex_ == -1) {
            currentIndex_ = 0;
            elementToParentIndexMap_.emplace(currentIndex_, -1);
            visitAndVisibleIndexSet_.insert(currentIndex_);
            WrapperElement(widget);
            widget.SetHierarchy(ROOT_HIERARCHY);
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
        LOG_I("select in widget, store current index is %{public}d", currentIndex_);
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

    void ElementNodeIteratorImpl::GetParentContainerBounds(Rect &dockerRect)
    {
        int tempParentIndex = currentIndex_;
        while (elementToParentIndexMap_.find(tempParentIndex) != elementToParentIndexMap_.cend()) {
            tempParentIndex = elementToParentIndexMap_.at(tempParentIndex);
            if (elementIndexToRectMap_.find(tempParentIndex) != elementIndexToRectMap_.cend()) {
                dockerRect = elementIndexToRectMap_.at(tempParentIndex);
                return;
            }
        }
    }

    void ElementNodeIteratorImpl::CheckAndUpdateContainerRectMap(const Rect &widgetRect)
    {
        if (CONTAINER_TYPE.find(elementInfoLists_[currentIndex_].GetComponentType()) != CONTAINER_TYPE.cend()) {
            elementIndexToRectMap_.emplace(currentIndex_, widgetRect);
        }
    }

    void ElementNodeIteratorImpl::RemoveInvisibleWidget()
    {
        visitAndVisibleIndexSet_.erase(currentIndex_);
    }

    std::string ElementNodeIteratorImpl::GenerateNodeHashCode(const AccessibilityElementInfo &element)
    {
        int winId = element.GetWindowId();
        int accessId = element.GetAccessibilityId();
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
        // 判断是否存在子
        if (childCount > 0) {
            if (visitAndVisibleIndexSet_.find(currentIndex_) == visitAndVisibleIndexSet_.cend()) {
                LOG_I("node %{public}d is invisible not find its children",
                      elementInfoLists_[currentIndex_].GetAccessibilityId());
            } else {
                for (int i = currentIndex_ + 1; i < elementInfoLists_.size(); ++i) {
                    if (elementInfoLists_[i].GetAccessibilityId() != elementInfoLists_[currentIndex_].GetChildId(0)) {
                        continue;
                    }
                    // 找到了第一个子在列表的位置
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
        }

        // 不存在子，就找兄弟
        if (elementToParentIndexMap_.find(currentIndex_) == elementToParentIndexMap_.cend()) {
            LOG_I("This node has no parent: %{public}d", elementInfoLists_[currentIndex_].GetAccessibilityId());
            return false;
        }
        int parentIndex = elementToParentIndexMap_.at(currentIndex_);
        int tempChildIndex = currentIndex_;
        if (elementToParentIndexMap_.find(topIndex_) == elementToParentIndexMap_.cend()) {
            LOG_I("This topIndex_ has no parent: %{public}d", topIndex_);
            return false;
        }
        while (parentIndex != elementToParentIndexMap_.at(topIndex_)) {
            AccessibilityElementInfo &parentModel = elementInfoLists_[parentIndex];
            int parentChildCount = parentModel.GetChildCount();
            // 找到当前node位置
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
                        LOG_E("Node info error, except: %{public}d, actual is %{public}d",
                              parentModel.GetChildId(i + 1),
                              elementInfoLists_[tempChildIndex + 1].GetAccessibilityId());
                    }
                }
            }
            // 未找到，继续查找父节点
            if (elementToParentIndexMap_.find(parentIndex) == elementToParentIndexMap_.cend()) {
                LOG_I("This node has no parent: %{public}d", elementInfoLists_[parentIndex].GetAccessibilityId());
                return false;
            }
            tempChildIndex = parentIndex;
            parentIndex = elementToParentIndexMap_.at(parentIndex);
        }
        // 已完成搜索
        return false;
    }

    void ElementNodeIteratorImpl::WrapperNodeAttrToVec(Widget &widget,
                                                                     const AccessibilityElementInfo &element)
    {
        // 存放原始的尺寸
        Accessibility::Rect nodeOriginRect = elementInfoLists_[currentIndex_].GetRectInScreen();
        Rect visibleRect{nodeOriginRect.GetLeftTopXScreenPostion(), nodeOriginRect.GetRightBottomXScreenPostion(),
                         nodeOriginRect.GetLeftTopYScreenPostion(), nodeOriginRect.GetRightBottomYScreenPostion()};
        widget.SetBounds(visibleRect);

        // 属性处理
        widget.SetAttr(UiAttr::ACCESSIBILITY_ID, std::to_string(element.GetAccessibilityId()));
        widget.SetAttr(UiAttr::ID, element.GetInspectorKey());
        widget.SetAttr(UiAttr::TEXT, element.GetContent());
        widget.SetAttr(UiAttr::KEY, element.GetInspectorKey());
        // Description怎么处理
        widget.SetAttr(UiAttr::TYPE, element.GetComponentType());
        if (element.GetComponentType() == "rootdecortag" || element.GetInspectorKey() == "ContainerModalTitleRow") {
            widget.SetAttr(UiAttr::TYPE, "DecorBar");
        }
        //[left,top][right, bottom]
        stringstream boundStream;
        boundStream << "[" << visibleRect.left_ << "," << visibleRect.top_ << "][" << visibleRect.right_ << ","
                    << visibleRect.bottom_ << "]";
        widget.SetAttr(UiAttr::BOUNDS, boundStream.str());
        widget.SetAttr(UiAttr::ENABLED, element.IsEnabled() ? "true" : "false");
        widget.SetAttr(UiAttr::FOCUSED, element.IsFocused() ? "true" : "false");
        widget.SetAttr(UiAttr::SELECTED, element.IsSelected() ? "true" : "false");
        widget.SetAttr(UiAttr::CLICKABLE, "false");
        widget.SetAttr(UiAttr::LONG_CLICKABLE, "false");
        widget.SetAttr(UiAttr::SCROLLABLE, "false");
        widget.SetAttr(UiAttr::CHECKABLE, element.IsCheckable() ? "true" : "false");
        widget.SetAttr(UiAttr::CHECKED, element.IsChecked() ? "true" : "false");
        widget.SetAttr(UiAttr::HOST_WINDOW_ID, std::to_string(element.GetWindowId()));
        widget.SetAttr(UiAttr::ORIGBOUNDS, boundStream.str());
        widget.SetAttr(UiAttr::HASHCODE, ElementNodeIteratorImpl::GenerateNodeHashCode(element));
        // 后续处理会根据实际是否可见rect刷新为true
        if (!element.IsVisible()) {
            LOG_I("widget %{public}s is not visible", widget.GetAttr(UiAttr::ACCESSIBILITY_ID).data());
        }
        widget.SetAttr(UiAttr::VISIBLE, element.IsVisible() ? "true" : "false");
        // 是否只有根节点需要abilityName和bundleName以及pagePath
        const auto app = element.GetBundleName();
        widget.SetAttr(UiAttr::BUNDLENAME, app);
        // 刷新action属性
        auto actions = element.GetActionList();
        for (auto &action : actions) {
            switch (action.GetActionType()) {
                case ACCESSIBILITY_ACTION_CLICK:
                    widget.SetAttr(UiAttr::CLICKABLE, "true");
                    break;
                case ACCESSIBILITY_ACTION_LONG_CLICK:
                    widget.SetAttr(UiAttr::LONG_CLICKABLE, "true");
                    break;
                case ACCESSIBILITY_ACTION_SCROLL_FORWARD:
                case ACCESSIBILITY_ACTION_SCROLL_BACKWARD:
                    widget.SetAttr(UiAttr::SCROLLABLE, "true");
                    break;
                default:
                    break;
            }
        }
    }
} // namespace OHOS::uitest
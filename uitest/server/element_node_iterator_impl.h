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

#ifndef ELEMENT_NODE_ITERATOR_IMPL_H
#define ELEMENT_NODE_ITERATOR_IMPL_H

#include "ui_model.h"
#include "element_node_iterator.h"
#include "accessibility_ui_test_ability.h"
#include "accessibility_element_info.h"

namespace OHOS::uitest {
    class ElementNodeIteratorImpl : public ElementNodeIterator {
    public:
        explicit ElementNodeIteratorImpl(const std::vector<OHOS::Accessibility::AccessibilityElementInfo> &elements);
        ElementNodeIteratorImpl();
        ~ElementNodeIteratorImpl() override;
        bool DFSNextWithInTarget(Widget &widget) override;
        bool DFSNext(Widget &widget) override;
        bool IsVisitFinish() const override;
        void RestoreNodeIndexByAnchor() override;
        void ResetNodeIndexToAnchor() override;
        void ClearDFSNext() override;
        void GetParentContainerBounds(Rect &dockerRect) override;
        void CheckAndUpdateContainerRectMap() override;
        void RemoveInvisibleWidget() override;
        static std::string GenerateNodeHashCode(const OHOS::Accessibility::AccessibilityElementInfo &element);

    protected:
        void WrapperElement(Widget &widget) override;

    private:
        bool VisitNodeByChildAndBrother(Widget &widget);
        bool VisitChildren(Widget& widget);
        bool VisitBrother(Widget &widget, int parentIndex, int tempCurrentIndex);
        void WrapperNodeAttrToVec(Widget &widget, const OHOS::Accessibility::AccessibilityElementInfo &element);
        void WrapperNodeActionAttrToVec(Widget &widget, const OHOS::Accessibility::AccessibilityElementInfo &element);
        std::vector<OHOS::Accessibility::AccessibilityElementInfo> elementInfoLists_;
    };
} // namespace OHOS::uitest

#endif
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

#ifndef ELEMENT_NODE_ITERATOR_H
#define ELEMENT_NODE_ITERATOR_H

#include "ui_model.h"

namespace OHOS::uitest {
    class ElementNodeIterator {
    public:
        ElementNodeIterator() = default;
        virtual bool DFSNextWithInTarget(Widget &widget) = 0;
        virtual bool DFSNext(Widget &widget) = 0;
        virtual bool IsVisitFinish() const = 0;
        virtual void RestoreNodeIndexByAnchor() = 0;
        virtual void ResetNodeIndexToAnchor() = 0;
        virtual void ClearDFSNext() = 0;
        virtual void GetParentContainerBounds(Rect &dockerRect) = 0;
        virtual void CheckAndUpdateContainerRectMap() = 0;
        virtual void RemoveInvisibleWidget() = 0;

        virtual ~ElementNodeIterator() = default;

    protected:
        virtual void WrapperElement(Widget &widget) = 0;
        int currentIndex_ = -1;
        int topIndex_ = 0;
        int lastCurrentIndex_ = -1;
        std::map<int, int> elementToParentIndexMap_;
        std::set<int> visitAndVisibleIndexSet_;
        std::map<int, Rect> elementIndexToRectMap_;
        std::map<int, std::string> elementIndexToHierarch_;
    };
} // namespace OHOS::uitest

#endif
/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef MOCK_ELEMENT_NODE_ITERATOR_H
#define MOCK_ELEMENT_NODE_ITERATOR_H

#include <iostream>
#include "element_node_iterator.h"
#include "json.hpp"

using json = nlohmann::json;

namespace OHOS::uitest {

    struct MockAccessibilityElementInfo {
        std::string accessibilityId = "-1";
        std::string inspectorKey = "";
        std::string content = "";
        std::string componentType = "";
        std::string bundleName = "";
        std::string hierarchy = "";
        bool enabled = true;
        bool focused = true;
        bool selected = true;
        bool checkable = true;
        bool checked = true;
        std::string windowId = "12";
        Rect rectInScreen = Rect{0, 0, 0, 0};
        int parentIndex = -1;
        std::vector<int> childIndexVec;
    };

    static void StringSplit(const string &str, const char split, std::vector<string> &res)
    {
        if (str == "") {
            return;
        }
        string strs = str + split;
        size_t pos = strs.find(split);

        while (pos != strs.npos) {
            std::string temp = strs.substr(0, pos);
            res.emplace_back(temp);
            strs = strs.substr(pos + 1, strs.size());
            pos = strs.find(split);
        }
    }

    static MockAccessibilityElementInfo WrapperJsonToElement(const json &obj)
    {
        MockAccessibilityElementInfo eleRoot;
        if (obj.find("accessibilityId") != obj.end()) {
            eleRoot.accessibilityId = obj["accessibilityId"];
        }
        if (obj.find("inspectorKey") != obj.end()) {
            eleRoot.inspectorKey = obj["inspectorKey"];
        }
        if (obj.find("content") != obj.end()) {
            eleRoot.content = obj["content"];
        }
        if (obj.find("componentType") != obj.end()) {
            eleRoot.componentType = obj["componentType"];
        }
        if (obj.find("bundleName") != obj.end()) {
            eleRoot.bundleName = obj["bundleName"];
        }
        if (obj.find("hierarchy") != obj.end()) {
            eleRoot.hierarchy = obj["hierarchy"];
        }
        if (obj.find("enabled") != obj.end()) {
            eleRoot.enabled = obj["enabled"] == "true" ? true : false;
        }
        if (obj.find("focused") != obj.end()) {
            eleRoot.focused = obj["focused"] == "true" ? true : false;
        }
        if (obj.find("selected") != obj.end()) {
            eleRoot.selected = obj["selected"] == "true" ? true : false;
        }
        if (obj.find("checkable") != obj.end()) {
            eleRoot.checkable = obj["checkable"] == "true" ? true : false;
        }
        if (obj.find("checked") != obj.end()) {
            eleRoot.checked = obj["checked"] == "true" ? true : false;
        }
        if (obj.find("windowId") != obj.end()) {
            eleRoot.windowId = obj["windowId"];
        }
        if (obj.find("rectInScreen") != obj.end()) {
            // 12,12,12,12
            std::string rectStr = obj["rectInScreen"];
            std::vector<std::string> rectStrVec;
            StringSplit(rectStr, ',', rectStrVec);
            eleRoot.rectInScreen.left_ = std::stoi(rectStrVec[0]);
            eleRoot.rectInScreen.right_ = std::stoi(rectStrVec[1]);
            eleRoot.rectInScreen.top_ = std::stoi(rectStrVec[2]);
            eleRoot.rectInScreen.bottom_ = std::stoi(rectStrVec[3]);
        }
        return eleRoot;
    }

    static void DFSVisitJson(std::vector<MockAccessibilityElementInfo> &eles, const json &obj, string hierarchy)
    {
        auto attributes = obj["attributes"];
        auto child = obj["children"];
        const size_t childCount = child.size();
        MockAccessibilityElementInfo info = WrapperJsonToElement(attributes);
        info.hierarchy = hierarchy;
        eles.emplace_back(info);
        for (size_t idx = 0; idx < childCount; idx++) {
            string childHierachy = WidgetHierarchyBuilder::Build(hierarchy, idx);
            DFSVisitJson(eles, child.at(idx), childHierachy);
        }
    }

    class MockElementNodeIterator : public ElementNodeIterator {
    public:
        static std::unique_ptr<MockElementNodeIterator> ConstructIteratorByJson(std::string domStr)
        {
            auto data = json::parse(domStr);
            auto attributes = data["attributes"];
            auto child = data["children"];
            std::vector<MockAccessibilityElementInfo> eles;
            DFSVisitJson(eles, data, "ROOT");
            for (int index = 0; index < eles.size(); ++index) {
                std::vector<int> childVec;
                for (int childIndex = index + 1; childIndex < eles.size(); ++childIndex) {
                    auto childHierarchy = eles[childIndex].hierarchy;
                    auto parentHie = WidgetHierarchyBuilder::GetParentWidgetHierarchy(childHierarchy);
                    if (parentHie == eles[index].hierarchy) {
                        eles[childIndex].parentIndex = index;
                        childVec.emplace_back(childIndex);
                    }
                }
                eles[index].childIndexVec = childVec;
            }
            return std::make_unique<MockElementNodeIterator>(eles);
        }
        MockElementNodeIterator() : ElementNodeIterator() {}
        MockElementNodeIterator(const std::vector<MockAccessibilityElementInfo> &elements) : ElementNodeIterator()
        {
            elementInfoLists_ = std::move(elements);
        }

        ~MockElementNodeIterator() = default;

        bool DFSNextWithInTarget(Widget &widget) override
        {
            if (elementInfoLists_[currentIndex_ + 1].parentIndex == elementInfoLists_[topIndex_].parentIndex) {
                return false;
            }
            if (currentIndex_ >= elementInfoLists_.size() - 1) {
                return false;
            }
            ++currentIndex_;
            WrapperElement(widget);
            return true;
        }

        bool DFSNext(Widget &widget) override
        {
            ++currentIndex_;
            if (currentIndex_ < elementInfoLists_.size()) {
                WrapperElement(widget);
                return true;
            }
            return false;
        }

        bool IsVisitFinish() const override
        {
            return elementInfoLists_.size() == elementToParentIndexMap_.size();
        }
        void RestoreNodeIndexByAnchor() override
        {
            topIndex_ = currentIndex_;
        }
        void ResetNodeIndexToAnchor() override
        {
            topIndex_ = 0;
        }
        void ClearDFSNext() override
        {
            elementToParentIndexMap_.clear();
            elementIndexToHierarch_.clear();
            visitAndVisibleIndexSet_.clear();
            currentIndex_ = -1;
            topIndex_ = 0;
        }
        void GetParentContainerBounds(Rect &dockerRect) override
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
        void CheckAndUpdateContainerRectMap() override
        {
            if (CONTAINER_TYPE.find(elementInfoLists_[currentIndex_].componentType) != CONTAINER_TYPE.cend()) {
                elementIndexToRectMap_.emplace(currentIndex_, elementInfoLists_[currentIndex_].rectInScreen);
            }
        }
        void RemoveInvisibleWidget() override
        {
            visitAndVisibleIndexSet_.erase(currentIndex_);
        }
        std::vector<MockAccessibilityElementInfo> elementInfoLists_;

    protected:
        std::string GenerateNodeHashCode(const MockAccessibilityElementInfo &element)
        {
            std::string winId = element.windowId;
            std::string accessId = element.accessibilityId;
            std::stringstream hashcodeStr;
            hashcodeStr << winId;
            hashcodeStr << ":";
            hashcodeStr << accessId;
            return hashcodeStr.str();
        }
        void WrapperNodeAttrToVec(Widget &widget, const MockAccessibilityElementInfo &element)
        {
            Rect nodeOriginRect = element.rectInScreen;
            widget.SetBounds(nodeOriginRect);

            // mock hierarchy
            widget.SetHierarchy(element.hierarchy);
            widget.SetAttr(UiAttr::ACCESSIBILITY_ID, element.accessibilityId);
            widget.SetAttr(UiAttr::ID, element.inspectorKey);
            widget.SetAttr(UiAttr::BUNDLENAME, element.bundleName);
            widget.SetAttr(UiAttr::TEXT, element.content);
            widget.SetAttr(UiAttr::KEY, element.inspectorKey);
            widget.SetAttr(UiAttr::TYPE, element.componentType);
            if (element.inspectorKey == "rootdecortag" || element.inspectorKey == "ContainerModalTitleRow") {
                widget.SetAttr(UiAttr::TYPE, "DecorBar");
            }
            // [left,top][right, bottom]
            stringstream boundStream;
            boundStream << "[" << nodeOriginRect.left_ << "," << nodeOriginRect.top_ << "][" << nodeOriginRect.right_
                        << "," << nodeOriginRect.bottom_ << "]";
            widget.SetAttr(UiAttr::ENABLED, element.enabled ? "true" : "false");
            widget.SetAttr(UiAttr::FOCUSED, element.focused ? "true" : "false");
            widget.SetAttr(UiAttr::SELECTED, element.selected ? "true" : "false");
            widget.SetAttr(UiAttr::CLICKABLE, "false");
            widget.SetAttr(UiAttr::LONG_CLICKABLE, "false");
            widget.SetAttr(UiAttr::SCROLLABLE, "false");
            widget.SetAttr(UiAttr::CHECKABLE, element.checkable ? "true" : "false");
            widget.SetAttr(UiAttr::CHECKED, element.checked ? "true" : "false");
            widget.SetAttr(UiAttr::HOST_WINDOW_ID, element.windowId);
            widget.SetAttr(UiAttr::ORIGBOUNDS, boundStream.str());
            widget.SetAttr(UiAttr::HASHCODE, GenerateNodeHashCode(element));
            widget.SetAttr(UiAttr::VISIBLE,
                           nodeOriginRect.GetHeight() > 0 && nodeOriginRect.GetWidth() > 0 ? "true" : "false");
        }
        void WrapperElement(Widget &widget) override
        {
            MockAccessibilityElementInfo element = elementInfoLists_[currentIndex_];
            WrapperNodeAttrToVec(widget, element);
        }
    };
} // namespace OHOS::uitest
#endif
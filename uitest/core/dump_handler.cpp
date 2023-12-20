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

#include "json.hpp"
#include "ui_model.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    static string_view GetMiddleStr(string_view str, size_t &index, string_view startStr, string_view endStr)
    {
        size_t ori = index;
        auto begin = str.find(startStr, index);
        if (begin != string::npos) {
            index = begin + startStr.size();
            auto end = str.find(endStr, index);
            if (end != string::npos) {
                string_view result = str.substr(index, end - index);
                index = end;
                return result;
            }
        }
        index = ori;
        return "";
    }

    static void DFSMarshalWidget(std::vector<Widget> &allWidget, int index, nlohmann::json &dom,
        const std::map<std::string, int> &widgetChildCountMap, std::map<std::string, int> &visitWidgetMap)
    {
        auto attrData = json();
        allWidget.at(index).WrapperWidgetToJson(attrData);
        auto childrenData = json::array();
        int childIndex = 0;
        int childCount = 0;
        int childVisit = 0;
        auto hierarchy = allWidget.at(index).GetHierarchy();
        if (widgetChildCountMap.find(hierarchy) != widgetChildCountMap.cend()) {
            childCount = widgetChildCountMap.at(hierarchy);
        }
        while (childVisit < childCount) {
            auto tempChildHierarchy = WidgetHierarchyBuilder::GetChildHierarchy(hierarchy, childIndex);
            ++childIndex;
            if (visitWidgetMap.find(tempChildHierarchy) == visitWidgetMap.cend()) {
                continue;
            }
            auto childWidIndex = visitWidgetMap.at(tempChildHierarchy);
            if (!allWidget.at(childWidIndex).IsVisible()) {
                ++childVisit;
                continue;
            }
            auto childData = json();
            DFSMarshalWidget(allWidget, childWidIndex, childData, widgetChildCountMap, visitWidgetMap);
            childrenData.emplace_back(childData);
            ++childVisit;
        }
        dom["attributes"] = attrData;
        dom["children"] = childrenData;
    }

    void DumpHandler::AddExtraAttrs(nlohmann::json &root, const map<int32_t, string_view> &elementTrees, size_t index)
    {
        auto windowIdValue = root["attributes"]["hostWindowId"].dump();
        auto windowId = atoi(windowIdValue.substr(1, windowIdValue.size() - 2).c_str());
        auto find = elementTrees.find(windowId);
        auto elementTree = (find != elementTrees.end()) ? find->second : "";
        string_view nodeEndStr = "|->";
        auto accessibilityIdInfo = root["attributes"]["accessibilityId"].dump();
        auto accessibilityId = accessibilityIdInfo.substr(1, accessibilityIdInfo.size() - 2);
        string_view accessibilityIdStr = "AccessibilityId: " + accessibilityId;
        string_view nodeAttrStr = GetMiddleStr(elementTree, index, accessibilityIdStr, nodeEndStr);
        auto extraAttrs = json();
        size_t nodeAttrTraverlIndex = 0;
        auto backgroundColor = GetMiddleStr(nodeAttrStr, nodeAttrTraverlIndex, "BackgroundColor: ", "\n");
        auto content = GetMiddleStr(nodeAttrStr, nodeAttrTraverlIndex, "Content: ", "\n");
        auto fontColor = GetMiddleStr(nodeAttrStr, nodeAttrTraverlIndex, "FontColor: ", "\n");
        auto fontSize = GetMiddleStr(nodeAttrStr, nodeAttrTraverlIndex, "FontSize: ", "\n");
        if (!backgroundColor.empty()) {
            extraAttrs["BackgroundColor"] = backgroundColor;
        }
        if (!content.empty()) {
            extraAttrs["Content"] = move(content);
        }
        if (!fontColor.empty()) {
            extraAttrs["FontColor"] = move(fontColor);
        }
        if (!fontSize.empty()) {
            extraAttrs["FontSize"] = move(fontSize);
        }
        if (!extraAttrs.empty()) {
            root["extraAttrs"] = extraAttrs;
        }
        auto &childrenData = root["children"];
        auto childCount = childrenData.size();
        for (size_t idx = 0; idx < childCount; idx++) {
            auto &child = childrenData.at(idx);
            AddExtraAttrs(child, elementTrees, index);
        }
    }

    void DumpHandler::DumpWindowInfoToJson(vector<Widget> &allWidget, nlohmann::json &root)
    {
        std::map<std::string, int> visitWidgetMap;
        std::map<std::string, int> widgetCountMap;
        for (int i = 0; i < allWidget.size(); ++i) {
            const Widget &wid = allWidget.at(i);
            std::string hie = wid.GetHierarchy();
            visitWidgetMap.emplace(hie, i);
            std::string parentHie = WidgetHierarchyBuilder::GetParentWidgetHierarchy(hie);
            if (widgetCountMap.find(parentHie) == widgetCountMap.cend()) {
                widgetCountMap[parentHie] = 1;
            } else {
                widgetCountMap[parentHie] = widgetCountMap[parentHie] + 1;
            }
        }
        DFSMarshalWidget(allWidget, 0, root, widgetCountMap, visitWidgetMap);
    }
} // namespace OHOS::uitest
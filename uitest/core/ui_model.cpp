/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include <algorithm>
#include "ui_model.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    void Widget::SetBounds(const Rect &bounds)
    {
        bounds_ = bounds;
    }

    string Widget::ToStr() const
    {
        stringstream os;
        os << "Widget{";
        for (int i = 0; i < UiAttr::MAX; i++) {
            os << ATTR_NAMES[i] << "='" << attributeVec_[i] << "',";
        }
        os << "}";
        return os.str();
    }

    unique_ptr<Widget> Widget::Clone(string_view hierarchy) const
    {
        auto clone = make_unique<Widget>(hierarchy);
        clone->bounds_ = this->bounds_;
        clone->attributeVec_ = attributeVec_;
        return clone;
    }

    std::vector<std::string> Widget::GetAttrVec() const
    {
        std::vector<std::string> retVec = attributeVec_;
        if (attributeVec_[UiAttr::BOUNDS].empty()) {
            stringstream boundStream;
            boundStream << "[" << bounds_.left_ << "," << bounds_.top_ << "][" << bounds_.right_ << ","
                        << bounds_.bottom_ << "]";
            retVec[UiAttr::BOUNDS] = boundStream.str();
        }
        return retVec;
    }

    void Widget::SetAttr(UiAttr attrId, string value)
    {
        if (attrId >= UiAttr::MAX) {
            LOG_E("Error attrId %{public}d, check it", attrId);
            return;
        }
        attributeVec_[attrId] = value;
    }

    std::string Widget::GetAttr(UiAttr attrId) const
    {
        if (attrId >= UiAttr::MAX) {
            return "none";
        }
        if (attrId == UiAttr::BOUNDS && attributeVec_[UiAttr::BOUNDS].empty()) {
            stringstream boundStream;
            boundStream << "[" << bounds_.left_ << "," << bounds_.top_ << "][" << bounds_.right_ << ","
                        << bounds_.bottom_ << "]";
            return boundStream.str();
        }
        return attributeVec_[attrId];
    }

    bool Widget::MatchAttr(const WidgetMatchModel& matchModel) const
    {
        UiAttr attr = matchModel.attrName;
        std::string_view value = matchModel.attrValue;
        ValueMatchPattern pattern = matchModel.pattern;
        std::string_view attrValue = attributeVec_[attr];
        switch (pattern) {
            case ValueMatchPattern::EQ:
                return attrValue == value;
            case ValueMatchPattern::CONTAINS:
                return attrValue.find(value) != std::string_view::npos;
            case ValueMatchPattern::STARTS_WITH:
                return attrValue.find(value) == 0;
            case ValueMatchPattern::ENDS_WITH:
                if (attrValue.length() < value.length()) {
                    return false;
                }
                return attrValue.substr(attrValue.length() - value.length()) == value;
            default:
                break;
        }
        return false;
    }

    void Widget::SetHierarchy(const std::string &hierarch)
    {
        hierarchy_ = hierarch;
        attributeVec_[UiAttr::HIERARCHY] = hierarch;
    }

    void Widget::WrapperWidgetToJson(nlohmann::json &out)
    {
        for (int i = 0; i < UiAttr::HIERARCHY; ++i) {
            if (i == UiAttr::BOUNDS && attributeVec_[UiAttr::BOUNDS].empty()) {
                stringstream boundStream;
                boundStream << "[" << bounds_.left_ << "," << bounds_.top_ << "][" << bounds_.right_ << ","
                            << bounds_.bottom_ << "]";
                attributeVec_[i] = boundStream.str();
            }
            out[ATTR_NAMES[i].data()] = attributeVec_[i];
        }
        out[ATTR_NAMES[UiAttr::VISIBLE].data()] = attributeVec_[UiAttr::VISIBLE];
        out[ATTR_NAMES[UiAttr::HASHCODE].data()] = attributeVec_[UiAttr::HASHCODE];
        out[ATTR_NAMES[UiAttr::HINT].data()] = attributeVec_[UiAttr::HINT];
    }

    string WidgetHierarchyBuilder::Build(string_view parentWidgetHierarchy, uint32_t childIndex)
    {
        return string(parentWidgetHierarchy) + string(HIERARCHY_SEPARATOR) + to_string(childIndex);
    }

    string WidgetHierarchyBuilder::GetParentWidgetHierarchy(string_view hierarchy)
    {
        if (hierarchy == ROOT_HIERARCHY) {
            // no parent for root widget
            return "";
        }

        auto findRoot = hierarchy.find(ROOT_HIERARCHY);
        if (findRoot != 0) {
            // invalid hierarchy string
            return "";
        }
        auto findLastSeparator = hierarchy.find_last_of(HIERARCHY_SEPARATOR);
        if (findLastSeparator <= 0 || findLastSeparator == string::npos) {
            return "";
        }
        return string(hierarchy).substr(0, findLastSeparator);
    }

    string WidgetHierarchyBuilder::GetChildHierarchy(string_view hierarchy, uint32_t childIndex)
    {
        if (hierarchy.find(ROOT_HIERARCHY) != 0) {
            // invalid hierarchy string
            return "";
        }
        return string(hierarchy) + string(HIERARCHY_SEPARATOR) + to_string(childIndex);
    }
} // namespace OHOS::uitest

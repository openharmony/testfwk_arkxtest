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

#include "widget_image.h"
#include "ui_model.h"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;

    string WidgetImage::GetAttribute(string_view attrName, string_view defaultValue) const
    {
        auto item = attributes_.find(string(attrName));
        if (item == attributes_.end()) {
            return string(defaultValue);
        }
        return item->second;
    }

    void WidgetImage::SetAttributes(const map<string, string> &provider)
    {
        for (auto&[attr, value]:provider) {
            this->attributes_[attr] = value;
        }
    }

    void WidgetImage::SetSelectionDesc(string_view desc)
    {
        this->selectionCriteria_ = desc;
    }

    string WidgetImage::GetHierarchy() const
    {
        return attributes_.find(string(ATTR_HIERARCHY))->second;
    }

    string WidgetImage::GetHashCode() const
    {
        auto entry = attributes_.find(string(ATTR_HASHCODE));
        return entry == attributes_.end() ? "" : entry->second;
    }

    string WidgetImage::GetSelectionDesc() const
    {
        return selectionCriteria_;
    }

    bool WidgetImage::Compare(const WidgetImage &other) const
    {
        auto hashcode = this->GetHashCode();
        if (!hashcode.empty() && this->GetHashCode() == other.GetHashCode()) {
            return true;
        }
        for (auto&[attr, value]:attributes_) {
            if (attr == ATTR_HASHCODE) {
                continue;
            }
            auto entry = other.attributes_.find(attr);
            if (entry == other.attributes_.end() || value != entry->second) {
                return false;
            }
        }
        return true;
    }

    void WidgetImage::WriteIntoParcel(json &data) const
    {
        data["selection_criteria"] = selectionCriteria_;
        for (auto&[attr, value]:attributes_) {
            data[attr] = value;
        }
    }

    void WidgetImage::ReadFromParcel(const json &data)
    {
        for (auto&[key, value] : data.items()) {
            if (key == "selection_criteria") {
                selectionCriteria_ = value;
            } else {
                attributes_[key] = value;
            }
        }
    }
}


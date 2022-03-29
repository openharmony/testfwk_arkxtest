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

#ifndef UI_WIDGET_IMAGE_H
#define UI_WIDGET_IMAGE_H

#include <string>
#include <map>
#include "extern_api.h"

namespace OHOS::uitest {
    /**The informational image of a Widget on the WidgetTree.*/
    class WidgetImage : public ExternApi<TypeId::COMPONENT> {
    public:
        std::string GetAttribute(std::string_view attrName, std::string_view defaultValue) const;

        void SetAttributes(const std::map<std::string, std::string> &provider);

        void SetSelectionDesc(std::string_view desc);

        std::string GetSelectionDesc() const;

        /**Return the UI page scope identifier.*/
        std::string GetHierarchy() const;

        /**Return the hashcode of the represented widget object.*/
        std::string GetHashCode() const;

        /** Check if this WidgetImage equals to the given one. If the hashcode are same,
         * return true; else return true if only all the other attributes are same. */
        bool Compare(const WidgetImage &other) const;

        void WriteIntoParcel(nlohmann::json &data) const override;

        void ReadFromParcel(const nlohmann::json &data) override;

    private:
        // the attributes
        std::map <std::string, std::string> attributes_;
        // the description of the selection criteria of this widget.
        std::string selectionCriteria_;
    };
}

#endif

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

#include "external_calls.h"
#include "ability_manager_client.h"

namespace OHOS::uitest {

    std::vector<std::string> GetForeAbility()
    {
        std::vector<std::string> elements;
        std::string bundleName, abilityName;
        auto amcPtr = AAFwk::AbilityManagerClient::GetInstance();
        if (amcPtr == nullptr) {
            std::cout<<"AbilityManagerClient is nullptr"<<std::endl;
            abilityName = "";
            bundleName = "";
        } else {
            auto elementName = amcPtr->GetTopAbility();
            if (elementName.GetBundleName().empty()) {
                std::cout<<"GetTopAbility GetBundleName is nullptr"<<std::endl;
                bundleName = "";
            } else {
                bundleName = elementName.GetBundleName();
            }
            if (elementName.GetAbilityName().empty()) {
                std::cout<<"GetTopAbility GetAbilityName is nullptr"<<std::endl;
                abilityName = "";
            } else {
                abilityName = elementName.GetAbilityName();
            }
        }
        elements.push_back(bundleName);
        elements.push_back(abilityName);
        return elements;
    }
}
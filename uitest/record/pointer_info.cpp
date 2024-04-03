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

#include "pointer_info.h"

namespace OHOS::uitest {
    const std::string PointerInfo::EVENT_TYPE = "pointer";
    std::map <int32_t, std::string> PointerInfo::OP_TYPE = {
        {TouchOpt::OP_CLICK, "click"},
        {TouchOpt::OP_LONG_CLICK, "longClick"},
        {TouchOpt::OP_DOUBLE_CLICK, "doubleClick"},
        {TouchOpt::OP_SWIPE, "swipe"},
        {TouchOpt::OP_DRAG, "drag"},
        {TouchOpt::OP_FLING, "fling"},
        {TouchOpt::OP_PINCH, "pinch"},
        {TouchOpt::OP_HOME, "home"},
        {TouchOpt::OP_RECENT, "recent"},
        {TouchOpt::OP_RETURN, "back"},
    };
    nlohmann::json FingerInfo::WriteData()
    {
        auto data = nlohmann::json();
        data["X_POSI"] = std::to_string(firstTouchEventInfo.x);
        data["Y_POSI"] = std::to_string(firstTouchEventInfo.y);
        auto first_event_attr = firstTouchEventInfo.attributes;
        if (!first_event_attr.empty()) {
            data["W1_ID"] = first_event_attr[UiAttr::ID];
            data["W1_Type"] = first_event_attr[UiAttr::TYPE];
            data["W1_Text"] = first_event_attr[UiAttr::TEXT];
            data["W1_BOUNDS"] = first_event_attr[UiAttr::BOUNDS];
            data["W1_HIER"] = first_event_attr[UiAttr::HIERARCHY];
        }
        data["X2_POSI"] = lastTouchEventInfo.x != 0 ? std::to_string(lastTouchEventInfo.x) : "";
        data["Y2_POSI"] = lastTouchEventInfo.y != 0 ? std::to_string(lastTouchEventInfo.y) : "";
        auto last_event_attr = lastTouchEventInfo.attributes;
        if (!last_event_attr.empty()) {
            data["W2_ID"] = last_event_attr[UiAttr::ID];
            data["W2_Type"] = last_event_attr[UiAttr::TYPE];
            data["W2_Text"] = last_event_attr[UiAttr::TEXT];
            data["W2_BOUNDS"] = last_event_attr[UiAttr::BOUNDS];
            data["W2_HIER"] = last_event_attr[UiAttr::HIERARCHY];
        }
        data["LENGTH"] = std::to_string(stepLength);
        data["MAX_VEL"] = std::to_string(MAX_VELOCITY);
        data["VELO"] = std::to_string(velocity);
        data["direction.X"] = std::to_string(direction.GetX());
        data["direction.Y"] = std::to_string(direction.GetY());
        return data;
    }

    std::string FingerInfo::WriteWindowData(std::string actionType)
    {
        std::stringstream sout;
        auto first_event_attr = firstTouchEventInfo.attributes;
        auto last_event_attr = lastTouchEventInfo.attributes;
        if (actionType == "click" || actionType == "longClick" || actionType == "doubleClick") {
            sout << actionType << ": " ;
            if (first_event_attr.empty()) {
                sout <<" at Point(x:" << firstTouchEventInfo.x << ", y:" << firstTouchEventInfo.y << ") "<< "; ";
            } else if (first_event_attr[UiAttr::ID] != "" || first_event_attr[UiAttr::TEXT] != "") {
                sout << " at Widget( id: " << first_event_attr[UiAttr::ID] << ", "
                     << "text: " << first_event_attr[UiAttr::TEXT] << ", "
                     << "type: " << first_event_attr[UiAttr::TYPE] << ") "<< "; ";
            } else {
                sout <<" at Point(x:" << firstTouchEventInfo.x << ", y:" << firstTouchEventInfo.y << ") "<< "; ";
            }
        }
        // fling swipe drag
        if (first_event_attr.empty()) {
            sout << "from Point(x:" << firstTouchEventInfo.x << ", y:" << firstTouchEventInfo.y << ") ";
        } else if (first_event_attr[UiAttr::ID] != "" || first_event_attr[UiAttr::TEXT] != "") {
            sout << "from Widget(id: " << first_event_attr[UiAttr::ID] << ", "
                    << "type: " << first_event_attr[UiAttr::TYPE] << ", "
                    << "text: " << first_event_attr[UiAttr::TEXT] << ") " << "; ";
        } else {
            sout << "from Point(x:" << firstTouchEventInfo.x << ", y:" << firstTouchEventInfo.y << ") ";
        }
        if (last_event_attr.empty()) {
            sout << " to Point(x:" << lastTouchEventInfo.x << ", y:" << lastTouchEventInfo.y << ") " << "; ";
        } else if (last_event_attr[UiAttr::ID] != "" || last_event_attr[UiAttr::TEXT] != "") {
            sout << "to Widget(id: " << last_event_attr[UiAttr::ID] << ", "
                    << "type: " << last_event_attr[UiAttr::TYPE] << ", "
                    << "text: " << last_event_attr[UiAttr::TEXT] << ") " << "; ";
        } else {
            sout << " to Point(x:" << lastTouchEventInfo.x << ", y:" << lastTouchEventInfo.y << ") " << "; ";
        }
        if (actionType == "fling" || actionType == "swipe") {
            sout << "Off-hand speed:" << velocity << ", " << "Step length:" << stepLength << ";";
        }
        return sout.str();
    }

    nlohmann::json PointerInfo::WriteData()
    {
        auto data = nlohmann::json();
        data["EVENT_TYPE"] = EVENT_TYPE;
        data["OP_TYPE"] = OP_TYPE[touchOpt];
        data["fingerNumber"] = std::to_string(fingers.size());
        data["duration"] = duration;

        data["BUNDLE"] = bundleName;
        data["ABILITY"] = abilityName;
        data["direction.X"] = std::to_string(avgDirection.GetX());
        data["direction.Y"] = std::to_string(avgDirection.GetY());
        data["LENGTH"] = std::to_string(avgStepLength);
        data["VELO"] = std::to_string(avgVelocity);
        data["CENTER_X"] = centerSet.px_==0.0 && centerSet.px_ == 0.0 ? "" :std::to_string(centerSet.px_);
        data["CENTER_Y"] = centerSet.px_==0.0 && centerSet.px_ == 0.0 ? "" :std::to_string(centerSet.px_);

        auto fingerjson = nlohmann::json();
        for (auto& finger :fingers) {
            fingerjson.push_back(finger.WriteData());
        }
        data["fingerList"] = fingerjson;
        return data;
    }

    std::string PointerInfo::WriteWindowData()
    {
        std::stringstream sout;
        std::string actionType = OP_TYPE[touchOpt];
        sout << actionType << " , "
             << "fingerNumber:" << std::to_string(fingers.size()) << " , " ;
        int i = 1;
        for (auto& finger :fingers) {
            sout << std::endl;
            sout << "\t" << "finger" << i << ":" << finger.WriteWindowData(actionType) ;
            i++;
        }
        return sout.str();
    }
}
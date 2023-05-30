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
    
    nlohmann::json FingerInfo::WriteData(){
        auto data = nlohmann::json();
        data["X_POSI"] = std::to_string(firstTouchEventInfo.x);
        data["Y_POSI"] = std::to_string(firstTouchEventInfo.y);
        data["W1_ID"] = firstTouchEventInfo.attributes.count("id")?firstTouchEventInfo.attributes.find("id")->second : "";
        data["W1_Type"] = firstTouchEventInfo.attributes.count("type") ? firstTouchEventInfo.attributes.find("type")->second : "";
        data["W1_Text"] = firstTouchEventInfo.attributes.count("text") ? firstTouchEventInfo.attributes.find("text")->second : "";
        data["W1_BOUNDS"] = firstTouchEventInfo.attributes.count("bounds") ? firstTouchEventInfo.attributes.find("bounds")->second : "";
        data["W1_HIER"] = firstTouchEventInfo.attributes.count("hierarchy") ? firstTouchEventInfo.attributes.find("hierarchy")->second : "";

        data["X2_POSI"] = lastTouchEventInfo.x != 0 ? std::to_string(lastTouchEventInfo.x) : "";
        data["Y2_POSI"] = lastTouchEventInfo.y != 0 ? std::to_string(lastTouchEventInfo.y) : "";
        data["W2_ID"] = lastTouchEventInfo.attributes.count("id")?lastTouchEventInfo.attributes.find("id")->second : "";
        data["W2_Type"] = lastTouchEventInfo.attributes.count("type") ? lastTouchEventInfo.attributes.find("type")->second : "";
        data["W2_Text"] = lastTouchEventInfo.attributes.count("text") ? lastTouchEventInfo.attributes.find("text")->second : "";
        data["W2_BOUNDS"] = lastTouchEventInfo.attributes.count("bounds") ? lastTouchEventInfo.attributes.find("bounds")->second : "";
        data["W2_HIER"] = lastTouchEventInfo.attributes.count("hierarchy") ? lastTouchEventInfo.attributes.find("hierarchy")->second : "";

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
        if (actionType == "fling" || actionType == "swipe" || actionType == "drag") {
            if (firstTouchEventInfo.attributes.find("id")->second != "" || firstTouchEventInfo.attributes.find("text")->second != "") {
                sout << "from Widget(id: " << firstTouchEventInfo.attributes.find("id")->second << ", "
                     << "type: " << firstTouchEventInfo.attributes.find("type")->second << ", "
                     << "text: " << firstTouchEventInfo.attributes.find("text")->second << ") " << "; ";
            } else {
                sout << "from Point(x:" << firstTouchEventInfo.x << ", y:" << firstTouchEventInfo.y
                     << ") to Point(x:" << lastTouchEventInfo.x << ", y:" << lastTouchEventInfo.y << ") " << "; ";
            }
            if (actionType == "fling" || actionType == "swipe") {
                sout << "Off-hand speed:" << velocity << ", "
                     << "Step length:" << stepLength << ";";
            }
        } else if (actionType == "click" || actionType == "longClick" || actionType == "doubleClick") {
            sout << actionType << ": " ;
            if (firstTouchEventInfo.attributes.find("id")->second != "" || firstTouchEventInfo.attributes.find("text")->second != "") {
                sout << " at Widget( id: " << firstTouchEventInfo.attributes.find("id")->second << ", "
                     << "text: " << firstTouchEventInfo.attributes.find("text")->second << ", "
                     << "type: " << firstTouchEventInfo.attributes.find("type")->second<< ") "<< "; ";
            } else {
                sout <<" at Point(x:" << firstTouchEventInfo.x << ", y:" << firstTouchEventInfo.y << ") "<< "; ";
            }
        }
        return sout.str();
    }

    nlohmann::json PointerInfo::WriteData(){
        auto data = nlohmann::json();
        data["EVENT_TYPE"] = EVENT_TYPE;
        data["OP_TYPE"] = OP_TYPE[touchOpt];
        data["fingerNumber"] = std::to_string(fingers.size());
        data["duration"] = duration;

        data["BUNDLE"] = bundleName;
        data["ABILITY"] = abilityName;
        data["direction.X"] = std::to_string(avg_direction.GetX());
        data["direction.Y"] = std::to_string(avg_direction.GetY());
        data["LENGTH"] = std::to_string(avg_stepLength);
        data["VELO"] = std::to_string(avg_velocity);
        data["CENTER_X"] = center_set.GetX()==0.0 && center_set.GetY() == 0.0 ? "" :std::to_string(center_set.GetX());
        data["CENTER_Y"] = center_set.GetX()==0.0 && center_set.GetY() == 0.0 ? "" :std::to_string(center_set.GetY());

        auto fingerjson = nlohmann::json();
        for (auto& finger :fingers){
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
            //  << "BUNDLE:" << bundleName << " , "
            //  << "ABILITY" << abilityName << " , "
            //  << "direction.x" << std::to_string(avg_direction.GetX()) << " , "
            //  << "direction.y" << std::to_string(avg_direction.GetY()) << " , "
            //  << "LENGTH" << std::to_string(avg_stepLength) << " , "
            //  << "VELO" << std::to_string(avg_velocity.GetVeloValue()) << " , " << std::endl
        int i= 1;
        for (auto& finger :fingers){
            sout << std::endl;
            sout << "\t" << "finger" << i << ":" << finger.WriteWindowData(actionType) << "; " ;
            i++;
        }
        return sout.str();
    }
}



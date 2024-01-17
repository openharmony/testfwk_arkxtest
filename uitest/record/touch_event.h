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

#ifndef EVENT_TOUCH_EVENT_H
#define EVENT_TOUCH_EVENT_H

#include <list>
#include <map>
#include "offset.h"
#include "ui_model.h"

namespace OHOS::uitest {
using TimeStamp = std::chrono::high_resolution_clock::time_point;

struct TouchPoint final {
    int32_t id = 0;
    int32_t x = 0;
    int32_t y = 0;
    TimeStamp downTime;
};
/**
 * @brief TouchEvent contains the active change point and a list of all touch points.
 */
struct TouchEventInfo final {
    // the active changed point info
    // The ID is used to identify the point of contact between the finger and the screen. Different fingers have
    // different ids.
    int32_t id = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t wx = 0;
    int32_t wy = 0;
    // nanosecond time stamp.
    int64_t actionTime;
    int64_t downTime;
    double durationSeconds;
    std::vector<std::string> attributes;
    std::string bundleName;
    std::string abilityName;
    Point GetPoint() const
    {
        return Point(x, y);
    }
    TimeStamp GetActionTimeStamp() const
    {
        TimeStamp time {std::chrono::duration_cast<TimeStamp::duration>(std::chrono::nanoseconds(actionTime * 1000))};
        return time;
    }
    TimeStamp GetDownTimeStamp() const
    {
        TimeStamp time {std::chrono::duration_cast<TimeStamp::duration>(std::chrono::nanoseconds(downTime * 1000))};
        return time;
    }
    void Resets()
    {
        x = 0;
        y = 0;
        actionTime = GetCurrentMillisecond();
        downTime = GetCurrentMillisecond();
    }
};
} // namespace OHOS::uitest
#endif // EVENT_TOUCH_EVENT_H

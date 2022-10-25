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

#ifndef EVENT_TOUCH_EVENT_H
#define EVENT_TOUCH_EVENT_H

#include <list>
#include "offset.h"

namespace OHOS::uitest {
using TimeStamp = std::chrono::high_resolution_clock::time_point;

struct TouchPoint final {
    int32_t id = 0;
    float x = 0.0f;
    float y = 0.0f;
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
    float x = 0.0f;
    float y = 0.0f;
    // nanosecond time stamp.
    TimeStamp time;
    Offset GetOffset() const
    {
        return Offset(x, y);
    }
};
} // namespace OHOS::uitest
#endif // EVENT_TOUCH_EVENT_H

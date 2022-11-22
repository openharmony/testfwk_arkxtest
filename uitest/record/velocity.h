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

#ifndef GESTURES_VELOCITY_H
#define GESTURES_VELOCITY_H
#include "offset.h"

namespace OHOS::uitest {
class Velocity final {
public:
    Velocity() = default;
    explicit Velocity(const Offset& offsetPerSecond) : offsetPerSecond_(offsetPerSecond) {}
    ~Velocity() = default;

    void Resets()
    {
        offsetPerSecond_.Resets();
    }

    void SetOffsetPerSecond(const Offset& offsetPerSecond)
    {
        offsetPerSecond_ = offsetPerSecond;
    }

    Velocity operator+(const Velocity& velocity) const
    {
        return Velocity(offsetPerSecond_ + velocity.offsetPerSecond_);
    }

    Velocity operator-(const Velocity& velocity) const
    {
        return Velocity(offsetPerSecond_ - velocity.offsetPerSecond_);
    }

    Velocity operator*(double value) const
    {
        return Velocity(offsetPerSecond_ * value);
    }

    Velocity& operator+=(const Velocity& velocity)
    {
        offsetPerSecond_ += velocity.offsetPerSecond_;
        return *this;
    }

    bool operator==(const Velocity& velocity) const
    {
        return offsetPerSecond_ == velocity.offsetPerSecond_;
    }

    bool operator!=(const Velocity& velocity) const
    {
        return !operator==(velocity);
    }

    const Offset& GetOffsetPerSecond() const
    {
        return offsetPerSecond_;
    }
    
    double GetVeloX() const
    {
        return offsetPerSecond_.GetX();
    }

    double GetVeloY() const
    {
        return offsetPerSecond_.GetY();
    }

    double GetVeloValue() const
    {
        return offsetPerSecond_.GetDistance();
    }

private:
    Offset offsetPerSecond_;
};
} // namespace OHOS::uitest
#endif // GESTURES_VELOCITY_H

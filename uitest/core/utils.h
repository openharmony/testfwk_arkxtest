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

#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H
#include <cmath>
#include "common_utilities_hpp.h"
namespace OHOS::uitest {
inline bool NearEqual(const double left, const double right, const double epsilon)
{
    return (std::abs(left - right) <= epsilon);
}

template<typename T>
constexpr bool NearEqual(const T& left, const T& right);

template<>
inline bool NearEqual<float>(const float& left, const float& right)
{
    constexpr double epsilon = 0.001f;
    return NearEqual(left, right, epsilon);
}

template<>
inline bool NearEqual<double>(const double& left, const double& right)
{
    constexpr double epsilon = 0.00001f;
    return NearEqual(left, right, epsilon);
}

template<typename T>
constexpr bool NearEqual(const T& left, const T& right)
{
    return left == right;
}

inline bool NearZero(const double value, const double epsilon)
{
    return NearEqual(value, 0.0, epsilon);
}

inline bool NearEqual(const double left, const double right)
{
    constexpr double epsilon = 0.001f;
    return NearEqual(left, right, epsilon);
}

inline bool NearZero(const double left)
{
    constexpr double epsilon = 0.001f;
    return NearZero(left, epsilon);
}

inline bool LessOrEqual(double left, double right)
{
    constexpr double epsilon = 0.001f;
    return (left - right) < epsilon;
}

inline bool LessNotEqual(double left, double right)
{
    constexpr double epsilon = -0.001f;
    return (left - right) < epsilon;
}

inline bool GreatOrEqual(double left, double right)
{
    constexpr double epsilon = -0.001f;
    return (left - right) > epsilon;
}

inline bool GreatNotEqual(double left, double right)
{
    constexpr double epsilon = 0.001f;
    return (left - right) > epsilon;
}
} // namespace OHOS::uitest
#endif // UTILS_UTILS_H
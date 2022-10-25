/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <chrono>
#include <cmath>
#include <cstdint>

#include "common_utilities_hpp.h"

#define CHECK_NULL_VOID(ptr)                                            \
    do {                                                                \
        if (!(ptr)) {                                                   \
            LOG_W(#ptr " is null, return on line %{public}d", __LINE__); \
            return;                                                     \
        }                                                               \
    } while (0)

#define CHECK_NULL_RETURN(ptr, ret)                                     \
    do {                                                                \
        if (!(ptr)) {                                                   \
            LOG_W(#ptr " is null, return on line %{public}d", __LINE__); \
            return ret;                                                 \
        }                                                               \
    } while (0)

#define PRIMITIVE_CAT(x, y) x##y
#define CAT(x, y) PRIMITIVE_CAT(x, y)

#define COPY_SENTENCE(x) x = other.x;
#define LOOP_COPY(x) CAT(LOOP_COPY1 x, _END)
#define LOOP_COPY1(x) COPY_SENTENCE(x) LOOP_COPY2
#define LOOP_COPY2(x) COPY_SENTENCE(x) LOOP_COPY1
#define LOOP_COPY1_END
#define LOOP_COPY2_END

#define COMPARE_SENTENCE(x) (x == other.x)
#define LOOP_COMPARE(x) CAT(LOOP_COMPARE0 x, _END)
#define LOOP_COMPARE0(x) COMPARE_SENTENCE(x) LOOP_COMPARE1
#define LOOP_COMPARE1(x) &&COMPARE_SENTENCE(x) LOOP_COMPARE2
#define LOOP_COMPARE2(x) &&COMPARE_SENTENCE(x) LOOP_COMPARE1
#define LOOP_COMPARE1_END
#define LOOP_COMPARE2_END

#define DEFINE_COPY_CONSTRUCTOR(type) \
    type(const type& other)           \
    {                                 \
        *this = other;                \
    }

#define DEFINE_COPY_OPERATOR_WITH_PROPERTIES(type, PROPS) \
    type& operator=(const type& other)                    \
    {                                                     \
        if (&other != this) {                             \
            LOOP_COPY(PROPS)                              \
        }                                                 \
        return *this;                                     \
    }

#define DEFINE_COMPARE_OPERATOR_WITH_PROPERTIES(type, PROPS) \
    bool operator==(const type& other) const                 \
    {                                                        \
        if (&other != this) {                                \
            return true;                                     \
        }                                                    \
        return LOOP_COMPARE(PROPS);                          \
    }

#define DEFINE_COPY_CONSTRUCTOR_AND_COPY_OPERATOR_AND_COMPARE_OPERATOR_WITH_PROPERTIES(type, PROPS) \
    DEFINE_COPY_CONSTRUCTOR(type)                                                                   \
    DEFINE_COPY_OPERATOR_WITH_PROPERTIES(type, PROPS) DEFINE_COMPARE_OPERATOR_WITH_PROPERTIES(type, PROPS)

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

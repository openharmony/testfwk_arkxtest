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

#ifndef GEOMETRY_LEAST_SQUARE_IMPL_H
#define GEOMETRY_LEAST_SQUARE_IMPL_H

#include <vector>

#ifdef LINUX_PLATFORM
#include <cstdint>
#endif
namespace OHOS::uitest {
/**
 * @brief Least square method of four parametres.
 * the function template is a3 * x^3 + a2 * x^2 + a1 * x + a0 = y with four;
 * the function template is 0 * x^3 + a2 * x^2 + a1 * x + a0 = y with three.
 */
class LeastSquareImpl {
public:
    /**
     * @brief Construct a new Least Square Impl object.
     * @param paramsNum the right number is 4 or 3.
     */
    explicit LeastSquareImpl(int32_t paramsNum) : paramsNum_(paramsNum) {}

    /**
     * @brief Construct a new Least Square Impl object.
     * @param paramsNum the right number is 4 or 3.
     */
    LeastSquareImpl(int32_t paramsNum, int32_t countNum) : paramsNum_(paramsNum), countNum_(countNum) {}

    LeastSquareImpl() = default;
    ~LeastSquareImpl() = default;

    void UpdatePoint(double tVal, double pVal)
    {
        isResolved_ = false;
        tVals_.emplace_back(tVal);
        pVals_.emplace_back(pVal);
    }

    /**
     * @brief Set the Count Num which to compute.
     *
     * @param countNum the compute number.
     */
    void SetCountNum(int32_t countNum)
    {
        countNum_ = countNum;
    }

    /**
     * @brief Get the Least Square Params object
     *
     * @param params the four values of vector.
     * @return true get the least square result.
     * @return false failed to get the least square result.
     */
    bool GetLSMParams(std::vector<double>& params);

    inline const std::vector<double>& GetTVals() const
    {
        return tVals_;
    }

    inline const std::vector<double>& GetPVals() const
    {
        return pVals_;
    }

    inline int32_t GetTrackNum() const
    {
        return tVals_.size();
    }

    void Resets()
    {
        tVals_.clear();
        pVals_.clear();
        params_.clear();
        isResolved_ = false;
    }

private:
    std::vector<double> tVals_;
    std::vector<double> pVals_;
    std::vector<double> params_;
    int32_t paramsNum_ = 4;
    int32_t countNum_ = 4;
    bool isResolved_ = false;
};
} // namespace OHOS::uitest

#endif // GEOMETRY_LEAST_SQUARE_IMPL_H
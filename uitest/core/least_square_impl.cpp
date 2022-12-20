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

#include "least_square_impl.h"
#include "matrix3.h"
#include "matrix4.h"
#include "common_utilities_hpp.h"

namespace OHOS::uitest {
bool LeastSquareImpl::GetLSMParams(std::vector<double>& params)
{
    if (tVals_.size() <= ONE || (paramsNum_ != Matrix3::DIMENSION)) {
        LOG_E("size is invalid, %{public}d, %{public}d", static_cast<int32_t>(tVals_.size()), paramsNum_);
        return false;
    }
    params.resize(paramsNum_, ZERO);
    if (isResolved_) {
        params.assign(params_.begin(), params_.end());
        return true;
    }
    auto countNum = std::min(countNum_, static_cast<int32_t>(std::distance(tVals_.begin(), tVals_.end())));
    std::vector<double> xVals;
    xVals.resize(countNum, ZERO);
    std::vector<double> yVals;
    yVals.resize(countNum, ZERO);
    int32_t size = countNum - ONE;
    for (auto iter = tVals_.rbegin(); iter != tVals_.rend(); iter++) {
        xVals[size] = *iter;
        size--;
        if (size < ZERO) {
            break;
        }
    }
    size = countNum - ONE;
    for (auto iter = pVals_.rbegin(); iter != pVals_.rend(); iter++) {
        yVals[size] = *iter;
        size--;
        if (size < ZERO) {
            break;
        }
    }
    if (paramsNum_ == Matrix3::DIMENSION) {
        MatrixN3 matrixn3 { countNum };
        for (auto i = 0; i < countNum; i++) {
            const auto& value = xVals[i];
            matrixn3[i][TWO] = ONE;
            matrixn3[i][ONE] = value;
            matrixn3[i][ZERO] = value * value;
        }
        Matrix3 invert;
        auto transpose = matrixn3.Transpose();
        if (!(transpose * matrixn3).Invert(invert)) {
            LOG_E("fail to invert");
            return false;
        }
        auto matrix3n = invert * transpose;
        auto ret = matrix3n.ScaleMapping(yVals, params);
        if (ret) {
            params_.assign(params.begin(), params.end());
            isResolved_ = true;
        }
        return ret;
    }
    MatrixN4 matrixn4 { countNum };
    for (auto i = 0; i < countNum; i++) {
        const auto& value = xVals[i];
        matrixn4[i][THREE] = ONE;
        matrixn4[i][TWO] = value;
        matrixn4[i][ONE] = value * value;
        matrixn4[i][ZERO] = value * value * value;
    }
    auto transpose = matrixn4.Transpose();
    auto inversMatrix4 = Matrix4::Invert(transpose * matrixn4);
    auto matrix4n = inversMatrix4 * transpose;
    auto ret = matrix4n.ScaleMapping(yVals, params);
    if (ret) {
        params_.assign(params.begin(), params.end());
        isResolved_ = true;
    }
    return ret;
}
} // namespace OHOS::uitest

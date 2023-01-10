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

#include "matrix4.h"
#include <algorithm>
#include <cmath>
#include "utils.h"

namespace OHOS::uitest {
namespace {
constexpr int32_t MATRIX_LENGTH = Matrix4::DIMENSION * Matrix4::DIMENSION;
constexpr double ANGLE_UNIT = 0.017453f; // PI / 180
inline bool IsEqual(const double& left, const double& right)
{
    return NearEqual(left, right);
}
} // namespace

Matrix4 Matrix4::CreateIdentity()
{
    return Matrix4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4 Matrix4::CreateTranslate(double x, double y, double z)
{
    return Matrix4(1.0f, 0.0f, 0.0f, x, 0.0f, 1.0f, 0.0f, y, 0.0f, 0.0f, 1.0f, z, 0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4 Matrix4::CreateScale(double x, double y, double z)
{
    return Matrix4(x, 0.0f, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, 0.0f, z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4 Matrix4::CreateMatrix2D(double m00, double m10, double m01, double m11, double m03, double m13)
{
    return Matrix4(m00, m01, 0.0f, m03, m10, m11, 0.0f, m13, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4 Matrix4::CreateSkew(double x, double y)
{
    return Matrix4(1.0f, std::tan(x * ANGLE_UNIT), 0.0f, 0.0f, std::tan(y * ANGLE_UNIT), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4 Matrix4::CreatePerspective(double distance)
{
    auto result = CreateIdentity();
    if (GreatNotEqual(distance, 0.0f)) {
        result.matrix4x4_[TWO][THREE] = -1.0f / distance;
    }
    return result;
}

Matrix4 Matrix4::Invert(const Matrix4& matrix)
{
    Matrix4 inverted = CreateInvert(matrix);
    double determinant = matrix(ZERO, ZERO) * inverted(ZERO, ZERO) + matrix(ZERO, ONE) * inverted(ONE, ZERO) +
    matrix(ZERO, TWO) * inverted(TWO, ZERO) + matrix(ZERO, THREE) * inverted(THREE, ZERO);
    if (!NearZero(determinant)) {
        inverted = inverted * (1.0f / determinant);
    } else {
        inverted = CreateIdentity();
    }
    return inverted;
}

Matrix4::Matrix4()
    : Matrix4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f)
{}

Matrix4::Matrix4(const Matrix4& matrix)
{
    std::copy_n(&matrix.matrix4x4_[ZERO][ZERO], MATRIX_LENGTH, &matrix4x4_[ZERO][ZERO]);
}

Matrix4::Matrix4(double m00, double m01, double m02, double m03, double m10, double m11, double m12, double m13,
    double m20, double m21, double m22, double m23, double m30, double m31, double m32, double m33)
{
    matrix4x4_[ZERO][ZERO] = m00;
    matrix4x4_[ONE][ZERO] = m01;
    matrix4x4_[TWO][ZERO] = m02;
    matrix4x4_[THREE][ZERO] = m03;
    matrix4x4_[ZERO][ONE] = m10;
    matrix4x4_[ONE][ONE] = m11;
    matrix4x4_[TWO][ONE] = m12;
    matrix4x4_[THREE][ONE] = m13;
    matrix4x4_[ZERO][TWO] = m20;
    matrix4x4_[ONE][TWO] = m21;
    matrix4x4_[TWO][TWO] = m22;
    matrix4x4_[THREE][TWO] = m23;
    matrix4x4_[ZERO][THREE] = m30;
    matrix4x4_[ONE][THREE] = m31;
    matrix4x4_[TWO][THREE] = m32;
    matrix4x4_[THREE][THREE] = m33;
}

void Matrix4::SetEntry(int32_t row, int32_t col, double value)
{
    if ((row < ZERO || row >= DIMENSION) || (col < ZERO || col >= DIMENSION)) {
        return;
    }
    matrix4x4_[row][col] = value;
}

bool Matrix4::IsIdentityMatrix() const
{
    return *this == CreateIdentity();
}

int32_t Matrix4::Count()
{
    return MATRIX_LENGTH;
}

Matrix4 Matrix4::CreateInvert(const Matrix4& matrix)
{
    return Matrix4(
        matrix(ONE, ONE) * matrix(TWO, TWO) * matrix(THREE, THREE) - matrix(ONE, ONE) * matrix(TWO, THREE) *
        matrix(THREE, TWO) - matrix(TWO, ONE) * matrix(ONE, TWO) * matrix(THREE, THREE) + matrix(TWO, ONE) *
        matrix(ONE, THREE) * matrix(THREE, TWO) + matrix(THREE, ONE) * matrix(ONE, TWO) * matrix(TWO, THREE) -
        matrix(THREE, ONE) * matrix(ONE, THREE) * matrix(TWO, TWO),
        -matrix(ONE, ZERO) * matrix(TWO, TWO) * matrix(THREE, THREE) + matrix(ONE, ZERO) * matrix(TWO, THREE) *
        matrix(THREE, TWO) + matrix(TWO, ZERO) * matrix(ONE, TWO) * matrix(THREE, THREE) - matrix(TWO, ZERO) *
        matrix(ONE, THREE) * matrix(THREE, TWO) - matrix(THREE, ZERO) * matrix(ONE, TWO) * matrix(TWO, THREE) +
        matrix(THREE, ZERO) * matrix(ONE, THREE) * matrix(TWO, TWO),
        matrix(ONE, ZERO) * matrix(TWO, ONE) * matrix(THREE, THREE) - matrix(ONE, ZERO) * matrix(TWO, THREE) *
        matrix(THREE, ONE) - matrix(TWO, ZERO) * matrix(ONE, ONE) * matrix(THREE, THREE) + matrix(TWO, ZERO) *
        matrix(ONE, THREE) * matrix(THREE, ONE) + matrix(THREE, ZERO) * matrix(ONE, ONE) * matrix(TWO, THREE) -
        matrix(THREE, ZERO) * matrix(ONE, THREE) * matrix(TWO, ONE),
        -matrix(ONE, ZERO) * matrix(TWO, ONE) * matrix(THREE, TWO) + matrix(ONE, ZERO) * matrix(TWO, TWO) *
        matrix(THREE, ONE) + matrix(TWO, ZERO) * matrix(ONE, ONE) * matrix(THREE, TWO) - matrix(TWO, ZERO) *
        matrix(ONE, TWO) * matrix(THREE, ONE) - matrix(THREE, ZERO) * matrix(ONE, ONE) * matrix(TWO, TWO) +
        matrix(THREE, ZERO) * matrix(ONE, TWO) * matrix(TWO, ONE),
        -matrix(ZERO, ONE) * matrix(TWO, TWO) * matrix(THREE, THREE) + matrix(ZERO, ONE) * matrix(TWO, THREE) *
        matrix(THREE, TWO) + matrix(TWO, ONE) * matrix(ZERO, TWO) * matrix(THREE, THREE) - matrix(TWO, ONE) *
        matrix(ZERO, THREE) * matrix(THREE, TWO) - matrix(THREE, ONE) * matrix(ZERO, TWO) * matrix(TWO, THREE) +
        matrix(THREE, ONE) * matrix(ZERO, THREE) * matrix(TWO, TWO),
        matrix(ZERO, ZERO) * matrix(TWO, TWO) * matrix(THREE, THREE) - matrix(ZERO, ZERO) * matrix(TWO, THREE) *
        matrix(THREE, TWO) - matrix(TWO, ZERO) * matrix(ZERO, TWO) * matrix(THREE, THREE) + matrix(TWO, ZERO) *
        matrix(ZERO, THREE) * matrix(THREE, TWO) + matrix(THREE, ZERO) * matrix(ZERO, TWO) * matrix(TWO, THREE) -
        matrix(THREE, ZERO) * matrix(ZERO, THREE) * matrix(TWO, TWO),
        -matrix(ZERO, ZERO) * matrix(TWO, ONE) * matrix(THREE, THREE) + matrix(ZERO, ZERO) * matrix(TWO, THREE) *
        matrix(THREE, ONE) + matrix(TWO, ZERO) * matrix(ZERO, ONE) * matrix(THREE, THREE) - matrix(TWO, ZERO) *
        matrix(ZERO, THREE) * matrix(THREE, ONE) - matrix(THREE, ZERO) * matrix(ZERO, ONE) * matrix(TWO, THREE) +
        matrix(THREE, ZERO) * matrix(ZERO, THREE) * matrix(TWO, ONE),
        matrix(ZERO, ZERO) * matrix(TWO, ONE) * matrix(THREE, TWO) - matrix(ZERO, ZERO) * matrix(TWO, TWO) *
        matrix(THREE, ONE) - matrix(TWO, ZERO) * matrix(ZERO, ONE) * matrix(THREE, TWO) + matrix(TWO, ZERO) *
        matrix(ZERO, TWO) * matrix(THREE, ONE) + matrix(THREE, ZERO) * matrix(ZERO, ONE) * matrix(TWO, TWO) -
        matrix(THREE, ZERO) * matrix(ZERO, TWO) * matrix(TWO, ONE),
        matrix(ZERO, ONE) * matrix(ONE, TWO) * matrix(THREE, THREE) - matrix(ZERO, ONE) * matrix(ONE, THREE) *
        matrix(THREE, TWO) - matrix(ONE, ONE) * matrix(ZERO, TWO) * matrix(THREE, THREE) + matrix(ONE, ONE) *
        matrix(ZERO, THREE) * matrix(THREE, TWO) + matrix(THREE, ONE) * matrix(ZERO, TWO) * matrix(ONE, THREE) -
        matrix(THREE, ONE) * matrix(ZERO, THREE) * matrix(ONE, TWO),
        -matrix(ZERO, ZERO) * matrix(ONE, TWO) * matrix(THREE, THREE) + matrix(ZERO, ZERO) * matrix(ONE, THREE) *
        matrix(THREE, TWO) +  matrix(ONE, ZERO) * matrix(ZERO, TWO) * matrix(THREE, THREE) - matrix(ONE, ZERO) *
        matrix(ZERO, THREE) * matrix(THREE, TWO) - matrix(THREE, ZERO) * matrix(ZERO, TWO) * matrix(ONE, THREE) +
        matrix(THREE, ZERO) * matrix(ZERO, THREE) * matrix(ONE, TWO),
        matrix(ZERO, ZERO) * matrix(ONE, ONE) * matrix(THREE, THREE) - matrix(ZERO, ZERO) * matrix(ONE, THREE) *
        matrix(THREE, ONE) - matrix(ONE, ZERO) * matrix(ZERO, ONE) * matrix(THREE, THREE) + matrix(ONE, ZERO) *
        matrix(ZERO, THREE) * matrix(THREE, ONE) + matrix(THREE, ZERO) * matrix(ZERO, ONE) * matrix(ONE, THREE) -
        matrix(THREE, ZERO) * matrix(ZERO, THREE) * matrix(ONE, ONE),
        -matrix(ZERO, ZERO) * matrix(ONE, ONE) * matrix(THREE, TWO) + matrix(ZERO, ZERO) * matrix(ONE, TWO) *
        matrix(THREE, ONE) + matrix(ONE, ZERO) * matrix(ZERO, ONE) * matrix(THREE, TWO) - matrix(ONE, ZERO) *
        matrix(ZERO, TWO) * matrix(THREE, ONE) - matrix(THREE, ZERO) * matrix(ZERO, ONE) * matrix(ONE, TWO) +
        matrix(THREE, ZERO) * matrix(ZERO, TWO) * matrix(ONE, ONE),
        -matrix(ZERO, ONE) * matrix(ONE, TWO) * matrix(TWO, THREE) + matrix(ZERO, ONE) * matrix(ONE, THREE) *
        matrix(TWO, TWO) + matrix(ONE, ONE) * matrix(ZERO, TWO) * matrix(TWO, THREE) - matrix(ONE, ONE) *
        matrix(ZERO, THREE) * matrix(TWO, TWO) - matrix(TWO, ONE) * matrix(ZERO, TWO) * matrix(ONE, THREE)
        + matrix(TWO, ONE) * matrix(ZERO, THREE) * matrix(ONE, TWO),
        matrix(ZERO, ZERO) * matrix(ONE, TWO) * matrix(TWO, THREE) - matrix(ZERO, ZERO) * matrix(ONE, THREE) *
        matrix(TWO, TWO) - matrix(ONE, ZERO) * matrix(ZERO, TWO) * matrix(TWO, THREE) + matrix(ONE, ZERO) *
        matrix(ZERO, THREE) * matrix(TWO, TWO) + matrix(TWO, ZERO) * matrix(ZERO, TWO) * matrix(ONE, THREE) -
        matrix(TWO, ZERO) * matrix(ZERO, THREE) * matrix(ONE, TWO),
        -matrix(ZERO, ZERO) * matrix(ONE, ONE) * matrix(TWO, THREE) + matrix(ZERO, ZERO) * matrix(ONE, THREE) *
        matrix(TWO, ONE) + matrix(ONE, ZERO) * matrix(ZERO, ONE) * matrix(TWO, THREE) - matrix(ONE, ZERO) *
        matrix(ZERO, THREE) * matrix(TWO, ONE) - matrix(TWO, ZERO) * matrix(ZERO, ONE) * matrix(ONE, THREE) +
        matrix(TWO, ZERO) * matrix(ZERO, THREE) * matrix(ONE, ONE),
        matrix(ZERO, ZERO) * matrix(ONE, ONE) * matrix(TWO, TWO) - matrix(ZERO, ZERO) * matrix(ONE, TWO) *
        matrix(TWO, ONE) - matrix(ONE, ZERO) * matrix(ZERO, ONE) * matrix(TWO, TWO) + matrix(ONE, ZERO) *
        matrix(ZERO, TWO) * matrix(TWO, ONE) + matrix(TWO, ZERO) * matrix(ZERO, ONE) * matrix(ONE, TWO) -
        matrix(TWO, ZERO) * matrix(ZERO, TWO) * matrix(ONE, ONE));
}

bool Matrix4::operator==(const Matrix4& matrix) const
{
    return std::equal(&matrix4x4_[ZERO][ZERO], &matrix4x4_[ZERO][ZERO] + MATRIX_LENGTH,
                      &matrix.matrix4x4_[ZERO][ZERO], IsEqual);
}

Matrix4 Matrix4::operator*(double num)
{
    Matrix4 ret(*this);
    auto function = [num](double& v) { v *= num; };
    auto it = &ret.matrix4x4_[ZERO][ZERO];
    for (int32_t i = ZERO; i < MATRIX_LENGTH; ++it, ++i) {
        function(*it);
    }
    return ret;
}

Matrix4 Matrix4::operator*(const Matrix4& matrix)
{
    return Matrix4(
        matrix4x4_[ZERO][ZERO] * matrix(ZERO, ZERO) + matrix4x4_[ONE][ZERO] * matrix(ZERO, ONE) +
        matrix4x4_[TWO][ZERO] * matrix(ZERO, TWO) + matrix4x4_[THREE][ZERO] * matrix(ZERO, THREE),
        matrix4x4_[ZERO][ZERO] * matrix(ONE, ZERO) + matrix4x4_[ONE][ZERO] * matrix(ONE, ONE) +
        matrix4x4_[TWO][ZERO] * matrix(ONE, TWO) + matrix4x4_[THREE][ZERO] * matrix(ONE, THREE),
        matrix4x4_[ZERO][ZERO] * matrix(TWO, ZERO) + matrix4x4_[ONE][ZERO] * matrix(TWO, ONE) +
        matrix4x4_[TWO][ZERO] * matrix(TWO, TWO) + matrix4x4_[THREE][ZERO] * matrix(TWO, THREE),
        matrix4x4_[ZERO][ZERO] * matrix(THREE, ZERO) + matrix4x4_[ONE][ZERO] * matrix(THREE, ONE) +
        matrix4x4_[TWO][ZERO] * matrix(THREE, TWO) + matrix4x4_[THREE][ZERO] * matrix(THREE, THREE),
        matrix4x4_[ZERO][ONE] * matrix(ZERO, ZERO) + matrix4x4_[ONE][ONE] * matrix(ZERO, ONE) +
        matrix4x4_[TWO][ONE] * matrix(ZERO, TWO) + matrix4x4_[THREE][ONE] * matrix(ZERO, THREE),
        matrix4x4_[ZERO][ONE] * matrix(ONE, ZERO) + matrix4x4_[ONE][ONE] * matrix(ONE, ONE) +
        matrix4x4_[TWO][ONE] * matrix(ONE, TWO) + matrix4x4_[THREE][ONE] * matrix(ONE, THREE),
        matrix4x4_[ZERO][ONE] * matrix(TWO, ZERO) + matrix4x4_[ONE][ONE] * matrix(TWO, ONE) +
        matrix4x4_[TWO][ONE] * matrix(TWO, TWO) + matrix4x4_[THREE][ONE] * matrix(TWO, THREE),
        matrix4x4_[ZERO][ONE] * matrix(THREE, ZERO) + matrix4x4_[ONE][ONE] * matrix(THREE, ONE) +
        matrix4x4_[TWO][ONE] * matrix(THREE, TWO) + matrix4x4_[THREE][ONE] * matrix(THREE, THREE),
        matrix4x4_[ZERO][TWO] * matrix(ZERO, ZERO) + matrix4x4_[ONE][TWO] * matrix(ZERO, ONE) +
        matrix4x4_[TWO][TWO] * matrix(ZERO, TWO) + matrix4x4_[THREE][TWO] * matrix(ZERO, THREE),
        matrix4x4_[ZERO][TWO] * matrix(ONE, ZERO) + matrix4x4_[ONE][TWO] * matrix(ONE, ONE) +
        matrix4x4_[TWO][TWO] * matrix(ONE, TWO) + matrix4x4_[THREE][TWO] * matrix(ONE, THREE),
        matrix4x4_[ZERO][TWO] * matrix(TWO, ZERO) + matrix4x4_[ONE][TWO] * matrix(TWO, ONE) +
        matrix4x4_[TWO][TWO] * matrix(TWO, TWO) + matrix4x4_[THREE][TWO] * matrix(TWO, THREE),
        matrix4x4_[ZERO][TWO] * matrix(THREE, ZERO) + matrix4x4_[ONE][TWO] * matrix(THREE, ONE) +
        matrix4x4_[TWO][TWO] * matrix(THREE, TWO) + matrix4x4_[THREE][TWO] * matrix(THREE, THREE),
        matrix4x4_[ZERO][THREE] * matrix(ZERO, ZERO) + matrix4x4_[ONE][THREE] * matrix(ZERO, ONE) +
        matrix4x4_[TWO][THREE] * matrix(ZERO, TWO) + matrix4x4_[THREE][THREE] * matrix(ZERO, THREE),
        matrix4x4_[ZERO][THREE] * matrix(ONE, ZERO) + matrix4x4_[ONE][THREE] * matrix(ONE, ONE) +
        matrix4x4_[TWO][THREE] * matrix(ONE, TWO) + matrix4x4_[THREE][THREE] * matrix(ONE, THREE),
        matrix4x4_[ZERO][THREE] * matrix(TWO, ZERO) + matrix4x4_[ONE][THREE] * matrix(TWO, ONE) +
        matrix4x4_[TWO][THREE] * matrix(TWO, TWO) + matrix4x4_[THREE][THREE] * matrix(TWO, THREE),
        matrix4x4_[ZERO][THREE] * matrix(THREE, ZERO) + matrix4x4_[ONE][THREE] * matrix(THREE, ONE) +
        matrix4x4_[TWO][THREE] * matrix(THREE, TWO) + matrix4x4_[THREE][THREE] * matrix(THREE, THREE));
}

Matrix4N Matrix4::operator*(const Matrix4N& matrix) const
{
    int32_t columns = matrix.GetColNum();
    Matrix4N matrix4n { columns };
    for (auto i = 0; i < DIMENSION; i++) {
        for (auto j = 0; j < columns; j++) {
            double value = 0.0;
            for (auto k = 0; k < DIMENSION; k++) {
                value += matrix4x4_[i][k] * matrix[k][j];
            }
            matrix4n[i][j] = value;
        }
    }
    return matrix4n;
}

Point Matrix4::operator*(const Point& point)
{
    double x = point.GetX();
    double y = point.GetY();
    return Point(matrix4x4_[ZERO][ZERO] * x + matrix4x4_[ONE][ZERO] * y + matrix4x4_[THREE][ZERO],
        matrix4x4_[ZERO][ONE] * x + matrix4x4_[ONE][ONE] * y + matrix4x4_[THREE][ONE]);
}

Matrix4& Matrix4::operator=(const Matrix4& matrix)
{
    if (this == &matrix) {
        return *this;
    }
    std::copy_n(&matrix.matrix4x4_[ZERO][ZERO], MATRIX_LENGTH, &matrix4x4_[ZERO][ZERO]);
    return *this;
}

double Matrix4::operator[](int32_t index) const
{
    if (index < ZERO || index >= MATRIX_LENGTH) {
        return 0.0f;
    }
    int32_t row = index / DIMENSION;
    int32_t col = index % DIMENSION;
    return matrix4x4_[row][col];
}

double Matrix4::operator()(int32_t row, int32_t col) const
{
    // Caller guarantee row and col in range of [0, THREE].
    return matrix4x4_[row][col];
}

double Matrix4::Determinant() const
{
    if (this->IsIdentityMatrix()) {
        return 1.0;
    }

    double m00 = matrix4x4_[ZERO][ZERO];
    double m01 = matrix4x4_[ZERO][ONE];
    double m02 = matrix4x4_[ZERO][TWO];
    double m03 = matrix4x4_[ZERO][THREE];
    double m10 = matrix4x4_[ONE][ZERO];
    double m11 = matrix4x4_[ONE][ONE];
    double m12 = matrix4x4_[ONE][TWO];
    double m13 = matrix4x4_[ONE][THREE];
    double m20 = matrix4x4_[TWO][ZERO];
    double m21 = matrix4x4_[TWO][ONE];
    double m22 = matrix4x4_[TWO][TWO];
    double m23 = matrix4x4_[TWO][THREE];
    double m30 = matrix4x4_[THREE][ZERO];
    double m31 = matrix4x4_[THREE][ONE];
    double m32 = matrix4x4_[THREE][TWO];
    double m33 = matrix4x4_[THREE][THREE];

    double b00 = m00 * m11 - m01 * m10;
    double b01 = m00 * m12 - m02 * m10;
    double b02 = m00 * m13 - m03 * m10;
    double b03 = m01 * m12 - m02 * m11;
    double b04 = m01 * m13 - m03 * m11;
    double b05 = m02 * m13 - m03 * m12;
    double b06 = m20 * m31 - m21 * m30;
    double b07 = m20 * m32 - m22 * m30;
    double b08 = m20 * m33 - m23 * m30;
    double b09 = m21 * m32 - m22 * m31;
    double b10 = m21 * m33 - m23 * m31;
    double b11 = m22 * m33 - m23 * m32;

    return b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
}

void Matrix4::Transpose()
{
    std::swap(matrix4x4_[ZERO][ONE], matrix4x4_[ONE][ZERO]);
    std::swap(matrix4x4_[ZERO][TWO], matrix4x4_[TWO][ZERO]);
    std::swap(matrix4x4_[ZERO][THREE], matrix4x4_[THREE][ZERO]);
    std::swap(matrix4x4_[ONE][TWO], matrix4x4_[TWO][ONE]);
    std::swap(matrix4x4_[ONE][THREE], matrix4x4_[THREE][ONE]);
    std::swap(matrix4x4_[TWO][THREE], matrix4x4_[THREE][TWO]);
}

void Matrix4::ScaleMapping(const double src[DIMENSION], double dst[DIMENSION]) const
{
    double storage[DIMENSION];

    double* result = (src == dst) ? storage : dst;

    for (int i = 0; i < DIMENSION; i++) {
        double value = 0;
        for (int j = 0; j < DIMENSION; j++) {
            value += matrix4x4_[j][i] * src[j];
        }
        result[i] = value;
    }

    if (storage == result) {
        std::copy_n(result, DIMENSION, dst);
    }
}

std::string Matrix4::ToString() const
{
    std::string out;
    for (const auto& i : matrix4x4_) {
        for (double j : i) {
            out += std::to_string(j);
            out += ",";
        }
        out += "\n";
    }
    return out;
}

Matrix4N::Matrix4N(int32_t columns) : columns_(columns)
{
    matrix4n_.resize(DIMENSION, std::vector<double>(columns_, 0));
}

bool Matrix4N::SetEntry(int32_t row, int32_t col, double value)
{
    if (row >= DIMENSION || col >= columns_) {
        return false;
    }
    matrix4n_[row][col] = value;
    return true;
}

Matrix4 Matrix4N::operator*(const MatrixN4& matrix) const
{
    auto matrix4 = Matrix4::CreateIdentity();
    if (columns_ != matrix.GetRowNum()) {
        return matrix4;
    }
    for (int i = 0; i < DIMENSION; i++) {
        for (int j = 0; j < DIMENSION; j++) {
            double value = 0.0;
            for (int k = 0; k < columns_; k++) {
                value += matrix4n_[i][k] * matrix[k][j];
            }
            matrix4.SetEntry(i, j, value);
        }
    }
    return matrix4;
}

MatrixN4 Matrix4N::Transpose() const
{
    MatrixN4 matrix { columns_ };
    for (auto i = 0; i < DIMENSION; i++) {
        for (auto j = 0; j < columns_; j++) {
            matrix[j][i] = matrix4n_[i][j];
        }
    }
    return matrix;
}

std::vector<double> Matrix4N::ScaleMapping(const std::vector<double>& src) const
{
    std::vector<double> value { DIMENSION, 0 };
    if (static_cast<int32_t>(src.size()) != columns_) {
        return value;
    }
    for (int32_t i = 0; i < DIMENSION; i++) {
        double item = 0.0;
        for (int32_t j = 0; j < columns_; j++) {
            item = item + matrix4n_[i][j] * src[j];
        }
        value[i] = item;
    }
    return value;
}

bool Matrix4N::ScaleMapping(const std::vector<double>& src, std::vector<double>& result) const
{
    if (static_cast<int32_t>(src.size()) != columns_) {
        return false;
    }
    result.resize(DIMENSION, 0);
    for (int32_t i = 0; i < DIMENSION; i++) {
        double item = 0.0;
        for (int32_t j = 0; j < columns_; j++) {
            item = item + matrix4n_[i][j] * src[j];
        }
        result[i] = item;
    }
    return true;
}

MatrixN4::MatrixN4(int32_t rows) : rows_(rows)
{
    matrixn4_.resize(rows, std::vector<double>(DIMENSION, 0));
}

bool MatrixN4::SetEntry(int32_t row, int32_t col, double value)
{
    if (row >= rows_ || col >= DIMENSION) {
        return false;
    }
    matrixn4_[row][col] = value;
    return true;
}

Matrix4N MatrixN4::Transpose() const
{
    Matrix4N matrix { rows_ };
    for (auto i = 0; i < DIMENSION; i++) {
        for (auto j = 0; j < rows_; j++) {
            matrix[i][j] = matrixn4_[j][i];
        }
    }
    return matrix;
}

std::vector<double> MatrixN4::ScaleMapping(const std::vector<double>& src) const
{
    std::vector<double> value { rows_, 0 };
    if (static_cast<int32_t>(src.size()) != DIMENSION) {
        return value;
    }
    for (int32_t i = 0; i < rows_; i++) {
        double item = 0.0;
        for (int32_t j = 0; j < DIMENSION; j++) {
            item = item + matrixn4_[i][j] * src[j];
        }
        value[i] = item;
    }
    return value;
}
} // namespace OHOS::uitest
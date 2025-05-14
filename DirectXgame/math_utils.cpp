#include "math_utils.h" // または "geometry.h"
#include <cmath>
#include <DirectXMath.h> // DirectXMathを使用する場合

// 必要に応じて実装
Matrix4x4 MakeIdentity4x4() {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        result.m[i][i] = 1.0f;
    }
    return result;
}

Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
    Matrix4x4 result = MakeIdentity4x4();
    // 実装 (DirectXMath または自前で)
    // ...
    return result;
}

Matrix4x4 MakePerspectiveMatrix(float fovY, float aspectRatio, float nearZ, float farZ) {
    Matrix4x4 result = MakeIdentity4x4();
    // 実装 (DirectXMath または自前で)
    // ...
    return result;
}

Matrix4x4 makeOrthographicmMatrix(float left, float top, float right, float bottom, float nearZ, float farZ) {
    Matrix4x4 result = MakeIdentity4x4();
    // 実装 (DirectXMath または自前で)
    // ...
    return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result = MakeIdentity4x4();
    // 実装 (DirectXMath または自前で)
    // ...
    return result;
}

Matrix4x4 Inverse(const Matrix4x4& m) {
    Matrix4x4 result = MakeIdentity4x4();
    // 実装 (DirectXMath または自前で)
    // ...
    return result;
}

// ... 他の数学関数の実装
#pragma once

#include <cmath>

// 必要に応じてインライン展開
inline float Deg2Rad(float degree) { return degree * 3.14159265f / 180.0f; }

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vector4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f; // 同次座標
};

struct Matrix4x4 {
    float m[4][4] = {};
};

// 行列生成関数 (例)
Matrix4x4 MakeIdentity4x4();
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate);
Matrix4x4 MakePerspectiveMatrix(float fovY, float aspectRatio, float nearZ, float farZ);
Matrix4x4 makeOrthographicmMatrix(float left, float top, float right, float bottom, float nearZ, float farZ);

// 行列演算関数 (例)
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 Inverse(const Matrix4x4& m);

// ... 他の数学関数
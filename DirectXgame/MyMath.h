#pragma once

#include <cmath>

// Vector4の定義
struct Vector4
{
	float x, y, z, w;

	// コンストラクタ
	Vector4() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
	Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

// Vector3の定義
struct Vector3
{
	float x, y, z;

	// コンストラクタ
	Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

// Vector2の定義
struct Vector2
{
	float x, y;

	// コンストラクタ
	Vector2() : x(0.0f), y(0.0f) {}
	Vector2(float x, float y) : x(x), y(y) {}
};

// Matrix4x4の定義
struct Matrix4x4
{
	float m[4][4];

	// コンストラクタ
	Matrix4x4();
};

// 関数宣言(matrix4x4)

// 和
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

// 差
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

// 積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

// 逆行列
Matrix4x4 Inverse(const Matrix4x4& m);

// 転置行列
Matrix4x4 Transpose(const Matrix4x4& m);

// 単位行列
Matrix4x4 Identity4x4();

// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

//拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

//アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

//透視投影行列
Matrix4x4 MakePerspectiveMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

//正射影行列
Matrix4x4 makeOrthographicmMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

//ビューポート行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

// 関数宣言(vector3)

// 和
Vector3 Add(const Vector3& v1, const Vector3& v2);

// 差
Vector3 Substract(const Vector3& v1, const Vector3& v2);

// スカラー積
Vector3 Multiply(float scaler, const Vector3& v);

// 内積
float Dot(const Vector3& v1, const Vector3& v2);

// 長さ(ノルム)
float Length(const Vector3& v);

// 正規化
Vector3 Normalize(const Vector3& v);
#pragma once

/// <summary>
/// 3次元ベクトル
/// </summary>

struct Vector3 {
	float x;
	float y;
	float z;
};
// 足し算
Vector3 Add(const Vector3& v1, const Vector3& v2);

// 引き算
Vector3 Substract(const Vector3& v1, const Vector3& v2);

// スカラー積
Vector3 Multiply(float scaler, const Vector3& v);

// 内積
float Dot(const Vector3& v1, const Vector3& v2);

// 長さ(ノルム)
float Length(const Vector3& v);

// 正規化
Vector3 Normalize(const Vector3& v);

//クロス積
Vector3 Cross(const Vector3& v1, const Vector3& v2);
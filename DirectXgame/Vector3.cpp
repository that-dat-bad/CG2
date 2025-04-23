#include "Vector3.h"
#include <cmath>

Vector3 Add(const Vector3& v1, const Vector3& v2) {
	Vector3 buf;
	buf = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
	return buf;
}

Vector3 Substract(const Vector3& v1, const Vector3& v2) {
	Vector3 buf;
	buf = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	return buf;
}

Vector3 Multiply(float scaler, const Vector3& v) {
	Vector3 buf;
	buf = { scaler * v.x, scaler * v.y, scaler * v.z };
	return buf;
}

float Dot(const Vector3& v1, const Vector3& v2) {
	float result = { (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z) };
	return result;
}

float Length(const Vector3& v) {
	float result = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
	return result;
}

Vector3 Normalize(const Vector3& v) {
	Vector3 buf;
	buf = { v.x / Length(v), v.y / Length(v), v.z / Length(v) };
	return buf;
}


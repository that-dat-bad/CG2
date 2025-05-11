#include"Matrix4x4.h"
#include<cmath>
#include<assert.h>

Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 buf;

	buf.m[0][0] = m1.m[0][0] + m2.m[0][0];
	buf.m[0][1] = m1.m[0][1] + m2.m[0][1];
	buf.m[0][2] = m1.m[0][2] + m2.m[0][2];
	buf.m[0][3] = m1.m[0][3] + m2.m[0][3];
	buf.m[1][0] = m1.m[1][0] + m2.m[1][0];
	buf.m[1][1] = m1.m[1][1] + m2.m[1][1];
	buf.m[1][2] = m1.m[1][2] + m2.m[1][2];
	buf.m[1][3] = m1.m[1][3] + m2.m[1][3];
	buf.m[2][0] = m1.m[2][0] + m2.m[2][0];
	buf.m[2][1] = m1.m[2][1] + m2.m[2][1];
	buf.m[2][2] = m1.m[2][2] + m2.m[2][2];
	buf.m[2][3] = m1.m[2][3] + m2.m[2][3];
	buf.m[3][0] = m1.m[3][0] + m2.m[3][0];
	buf.m[3][1] = m1.m[3][1] + m2.m[3][1];
	buf.m[3][2] = m1.m[3][2] + m2.m[3][2];
	buf.m[3][3] = m1.m[3][3] + m2.m[3][3];

	return buf;
}

Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 buf;

	buf.m[0][0] = m1.m[0][0] - m2.m[0][0];
	buf.m[0][1] = m1.m[0][1] - m2.m[0][1];
	buf.m[0][2] = m1.m[0][2] - m2.m[0][2];
	buf.m[0][3] = m1.m[0][3] - m2.m[0][3];
	buf.m[1][0] = m1.m[1][0] - m2.m[1][0];
	buf.m[1][1] = m1.m[1][1] - m2.m[1][1];
	buf.m[1][2] = m1.m[1][2] - m2.m[1][2];
	buf.m[1][3] = m1.m[1][3] - m2.m[1][3];
	buf.m[2][0] = m1.m[2][0] - m2.m[2][0];
	buf.m[2][1] = m1.m[2][1] - m2.m[2][1];
	buf.m[2][2] = m1.m[2][2] - m2.m[2][2];
	buf.m[2][3] = m1.m[2][3] - m2.m[2][3];
	buf.m[3][0] = m1.m[3][0] - m2.m[3][0];
	buf.m[3][1] = m1.m[3][1] - m2.m[3][1];
	buf.m[3][2] = m1.m[3][2] - m2.m[3][2];
	buf.m[3][3] = m1.m[3][3] - m2.m[3][3];

	return buf;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 buf;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{

			buf.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] +
				m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
		}
	}
	return buf;

}

Matrix4x4 Inverse(const Matrix4x4& m)
{
	Matrix4x4 buf;

	//行列式
	float det =
		m.m[0][0] * (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
			m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
			m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) -
		m.m[0][1] * (m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
			m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
			m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) +
		m.m[0][2] * (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
			m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
			m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) -
		m.m[0][3] * (m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) -
			m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) +
			m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));


	// 行列式の逆数
	float invDet = 1.0f / det;

	// 各要素を計算し、行列式で割る
	buf.m[0][0] = invDet * (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
		m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
		m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]));

	buf.m[0][1] = invDet * -(m.m[0][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
		m.m[0][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
		m.m[0][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]));

	buf.m[0][2] = invDet * (m.m[0][1] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2]) -
		m.m[0][2] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1]) +
		m.m[0][3] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1]));

	buf.m[0][3] = invDet * -(m.m[0][1] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]) -
		m.m[0][2] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]) +
		m.m[0][3] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]));
	buf.m[1][0] = invDet * -(m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
		m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
		m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]));

	buf.m[1][1] = invDet * (m.m[0][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
		m.m[0][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
		m.m[0][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]));

	buf.m[1][2] = invDet * -(m.m[0][0] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2]) -
		m.m[0][2] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0]) +
		m.m[0][3] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0]));

	buf.m[1][3] = invDet * (m.m[0][0] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]) -
		m.m[0][2] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0]) +
		m.m[0][3] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]));

	buf.m[2][0] = invDet * (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
		m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
		m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

	buf.m[2][1] = invDet * -(m.m[0][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
		m.m[0][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
		m.m[0][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

	buf.m[2][2] = invDet * (m.m[0][0] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1]) -
		m.m[0][1] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0]) +
		m.m[0][3] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0]));

	buf.m[2][3] = invDet * -(m.m[0][0] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]) -
		m.m[0][1] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0]) +
		m.m[0][3] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]));

	buf.m[3][0] = invDet * -(m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) -
		m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) +
		m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

	buf.m[3][1] = invDet * (m.m[0][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) -
		m.m[0][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) +
		m.m[0][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

	buf.m[3][2] = invDet * -(m.m[0][0] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1]) -
		m.m[0][1] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0]) +
		m.m[0][2] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0]));

	buf.m[3][3] = invDet * (m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]) -
		m.m[0][1] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]) +
		m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]));




	return buf;
}

Matrix4x4 Transpose(const Matrix4x4& m)
{
	Matrix4x4 buf;
	buf.m[0][0] = m.m[0][0];
	buf.m[1][1] = m.m[1][1];
	buf.m[2][2] = m.m[2][2];
	buf.m[3][3] = m.m[3][3];


	buf.m[0][1] = m.m[1][0];
	buf.m[1][0] = m.m[0][1];
	buf.m[0][2] = m.m[2][0];
	buf.m[2][0] = m.m[0][2];
	buf.m[0][3] = m.m[3][0];
	buf.m[3][0] = m.m[0][3];
	buf.m[1][2] = m.m[2][1];
	buf.m[2][1] = m.m[1][2];
	buf.m[1][3] = m.m[3][1];
	buf.m[3][1] = m.m[1][3];
	buf.m[2][3] = m.m[3][2];
	buf.m[3][2] = m.m[2][3];

	return buf;
}

Matrix4x4 Identity4x4()
{
	Matrix4x4 buf;
	buf = { 0 };
	buf.m[0][0] = 1;
	buf.m[1][1] = 1;
	buf.m[2][2] = 1;
	buf.m[3][3] = 1;

	return buf;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate)
{

	Matrix4x4 buf;
	buf = Identity4x4();
	buf.m[3][0] = translate.x;
	buf.m[3][1] = translate.y;
	buf.m[3][2] = translate.z;
	return buf;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale)
{

	Matrix4x4 buf;
	buf = Identity4x4();
	buf.m[0][0] = scale.x;
	buf.m[1][1] = scale.y;
	buf.m[2][2] = scale.z;
	buf.m[3][3] = 1;
	return buf;
}

Matrix4x4 MakeRotateXMatrix(float radian)
{
	Matrix4x4 buf;
	buf = Identity4x4();
	buf.m[1][1] = std::cos(radian);
	buf.m[2][1] = -std::sin(radian);
	buf.m[1][2] = std::sin(radian);
	buf.m[2][2] = std::cos(radian);
	return buf;
}

Matrix4x4 MakeRotateYMatrix(float radian)
{
	Matrix4x4 buf;
	buf = Identity4x4();
	buf.m[0][0] = std::cos(radian);
	buf.m[2][0] = std::sin(radian);
	buf.m[0][2] = -std::sin(radian);
	buf.m[2][2] = std::cos(radian);
	return buf;
}

Matrix4x4 MakeRotateZMatrix(float radian)
{
	Matrix4x4 buf;
	buf = Identity4x4();
	buf.m[0][0] = std::cos(radian);
	buf.m[0][1] = std::sin(radian);
	buf.m[1][0] = -std::sin(radian);
	buf.m[1][1] = std::cos(radian);
	return buf;
}



Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	//拡大縮小行列
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

	// 回転行列の生成
	Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);


	// 平行移動行列
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);



	return Multiply(Multiply(Multiply(Multiply(scaleMatrix, rotateX), rotateY), rotateZ), translateMatrix);
}





static const int kRowHeight = 20;
static const int kColumnWidth = 60;

Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix)
{

	Vector3 buf;
	buf.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + matrix.m[3][0];
	buf.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + matrix.m[3][1];
	buf.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + matrix.m[3][2];
	float w = matrix.m[0][3] * vector.x + matrix.m[1][3] * vector.y + matrix.m[2][3] * vector.z + matrix.m[3][3];
	assert(w != 0.0f);
	buf.x /= w;
	buf.y /= w;
	buf.z /= w;
	return buf;
}

Matrix4x4 MakePerspectiveMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
	Matrix4x4 buf;

	buf = { 0 };
	buf.m[0][0] = (1 / aspectRatio) * (1 / tanf(fovY * 0.5f));
	buf.m[1][1] = 1 / tanf(fovY * 0.5f);
	buf.m[2][2] = farClip / (farClip - nearClip);
	buf.m[2][3] = 1;
	buf.m[3][2] = -(nearClip * farClip) / (farClip - nearClip);

	return buf;
}

Matrix4x4 makeOrthographicmMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)
{
	Matrix4x4 buf;
	buf = Identity4x4();
	buf.m[0][0] = 2 / (right - left);
	buf.m[1][1] = 2 / (top - bottom);
	buf.m[2][2] = 1 / (farClip - nearClip);
	buf.m[3][0] = (right + left) / (left - right);
	buf.m[3][1] = (top + bottom) / (bottom - top);
	buf.m[3][2] = nearClip / (nearClip - farClip);

	return buf;
}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
{
	Matrix4x4 buf;
	buf = Identity4x4();
	buf.m[0][0] = width / 2;
	buf.m[1][1] = -height / 2;
	buf.m[2][2] = maxDepth - minDepth;
	buf.m[3][0] = left + (width / 2);
	buf.m[3][1] = top + (height / 2);
	buf.m[3][2] = minDepth;
	return buf;
}


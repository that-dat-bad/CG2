#pragma once
#include <dinput.h>
#include "Matrix4x4.h" // プロジェクトに含まれるMatrix4x4のヘッダ
#include "Vector3.h"   // プロジェクトに含まれるVector3のヘッダ

class DebugCamera {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="windowWidth">ウィンドウの横幅</param>
    /// <param name="windowHeight">ウィンドウの縦幅</param>
    DebugCamera(int windowWidth, int windowHeight);

    /// <summary>
    /// 毎フレームの更新処理
    /// </summary>
    /// <param name="keyboard">キーボードデバイス</param>
    /// <param name="mouseState">マウスの状態</param>
    void Update(IDirectInputDevice8* keyboard, const DIMOUSESTATE& mouseState);

    /// <summary>
    /// ビュー行列の取得
    /// </summary>
    /// <returns>ビュー行列</returns>
    const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }

    /// <summary>
    /// プロジェクション行列の取得
    /// </summary>
    /// <returns>プロジェクション行列</returns>
    const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }

private:
    // ビュー行列を計算して更新する
    void UpdateViewMatrix();

private:
    // カメラのトランスフォーム
    Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translation_ = { 0.0f, 0.0f, -10.0f };

    // 行列
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;

    // マウスの回転感度
    float rotationSpeed_ = 0.003f;
    // 移動速度
    float moveSpeed_ = 0.1f;
};
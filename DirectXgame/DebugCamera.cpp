#include "DebugCamera.h"
#include <cmath> 
#include"Matrix4x4.h"
#include"Vector3.h"




DebugCamera::DebugCamera(int windowWidth, int windowHeight) {
    // 射影行列の初期化
    projectionMatrix_ = MakePerspectiveMatrix(0.45f, static_cast<float>(windowWidth) / static_cast<float>(windowHeight), 0.1f, 100.0f);
    // ビュー行列の初期化
    UpdateViewMatrix();
}

void DebugCamera::Update(IDirectInputDevice8* keyboard, const DIMOUSESTATE& mouseState) {
    bool isMoved = false;

    // --- マウスによる視点回転 ---
    // 右クリック中のみ回転
    if (mouseState.rgbButtons[1] & 0x80) {
        float dx = static_cast<float>(mouseState.lX) * rotationSpeed_;
        float dy = static_cast<float>(mouseState.lY) * rotationSpeed_;
        rotation_.y += dx;
        rotation_.x += dy;
        isMoved = true;
    }

    // --- キーボードによる移動 ---
    BYTE key[256] = {};
    keyboard->GetDeviceState(sizeof(key), key);

    // カメラの現在の向きから前方、右方ベクトルを計算
    Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotation_, { 0.0f, 0.0f, 0.0f });
    Vector3 forward = { cameraMatrix.m[2][0], cameraMatrix.m[2][1], cameraMatrix.m[2][2] };
    Vector3 right = { cameraMatrix.m[0][0], cameraMatrix.m[0][1], cameraMatrix.m[0][2] };

    Vector3 moveVector = { 0.0f, 0.0f, 0.0f };
    if (key[DIK_W]) { // 前進
        moveVector = Add(moveVector, forward);
        isMoved = true;
    }
    if (key[DIK_S]) { // 後退
        moveVector = Substract(moveVector, forward);
        isMoved = true;
    }
    if (key[DIK_A]) { // 左移動
        moveVector = Substract(moveVector, right);
        isMoved = true;
    }
    if (key[DIK_D]) { // 右移動
        moveVector = Add(moveVector, right);
        isMoved = true;
    }
    // 移動量を加算
    translation_ = Add(translation_, Multiply(moveSpeed_, moveVector));

    // --- マウスホイールによる前後移動 ---
    if (mouseState.lZ != 0) {
        float wheelMove = static_cast<float>(mouseState.lZ) / 120.0f * moveSpeed_ * 5.0f; // スクロール量を調整
        translation_ = Add(translation_, Multiply(wheelMove, forward));
        isMoved = true;
    }

    // カメラが移動・回転していたらビュー行列を更新
    if (isMoved) {
        UpdateViewMatrix();
    }
}

void DebugCamera::UpdateViewMatrix() {
    Matrix4x4 cameraMatrix = MakeAffineMatrix(scale_, rotation_, translation_);
    viewMatrix_ = Inverse(cameraMatrix);
}
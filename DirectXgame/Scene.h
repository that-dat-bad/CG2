#pragma once

#include <vector>
#include "MyMath.h"

// ゲームオブジェクトの基底クラス (仮)
class GameObject {
public:
    GameObject();
    virtual ~GameObject() = default;

    MyMath::Matrix4x4 worldTransform_; // ワールド変換行列
};

class Scene {
public:
    Scene();
    ~Scene();

    void Update();
    void Draw();

private:
    std::vector<GameObject*> gameObjects_; // シーン内のゲームオブジェクト
    // カメラやライトなどの情報もここに追加していく
};
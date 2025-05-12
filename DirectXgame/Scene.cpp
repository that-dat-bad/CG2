#include "Scene.h"

GameObject::GameObject() :
    worldTransform_(MyMath::MakeIdentity4x4()) {
}

Scene::Scene() {
}

Scene::~Scene() {
    for (GameObject* obj : gameObjects_) {
        delete obj;
    }
    gameObjects_.clear();
}

void Scene::Update() {
    // ゲームオブジェクトの更新処理など
    for (GameObject* obj : gameObjects_) {
        // obj->Update(); // 各オブジェクト固有の更新処理 (未実装)
    }
}

void Scene::Draw() {
    // ゲームオブジェクトの描画処理など
    for (GameObject* obj : gameObjects_) {
        // obj->Draw(); // 各オブジェクト固有の描画処理 (未実装)
    }
}
#pragma once
#include <Windows.h>
#include <cstdint>

#include "./externals/imgui/imgui.h"
#include "./externals/imgui/imgui_impl_dx12.h"
#include "./externals/imgui/imgui_impl_win32.h"

class WinApp {
public:
    static const int32_t kClientWidth = 1280;
    static const int32_t kClientHeight = 720;

    // シングルトンインスタンスの取得
    static WinApp* GetInstance();

    // 初期化
    void Initialize();

    // 更新
    bool Update();


    // HWNDの取得
    HWND GetHwnd() const { return hwnd_; }
    HINSTANCE GetHinstance() const { return wc_.hInstance; }

private:
    WinApp(const WinApp&) = delete;
    const WinApp& operator=(const WinApp&) = delete;

    // ウィンドウプロシージャ
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND hwnd_ = nullptr;
    WNDCLASS wc_{};
};
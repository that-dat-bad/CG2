#include "window.h"
#include <format>

Window::Window(HINSTANCE hInstance, const std::wstring& title, int32_t width, int32_t height)
    : hInstance_(hInstance) {

    WNDCLASS wc{};
    wc.lpfnWndProc = windowProc;
    wc.lpszClassName = L"CG2WindowClass";
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    RECT wrc = { 0, 0, width, height };
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    hwnd_ = CreateWindow(
        wc.lpszClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        hInstance_,
        nullptr);

    ShowWindow(hwnd_, SW_SHOW);
}

Window::~Window() {
    DestroyWindow(hwnd_);
    UnregisterClass(L"CG2WindowClass", hInstance_);
}

bool Window::processMessage() {
    MSG msg{};
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.message != WM_QUIT;
}

LRESULT CALLBACK Window::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    //ImGuiの処理は一旦ここに書く（後で移動）
    //if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    //{
    //    return true;
    //}

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
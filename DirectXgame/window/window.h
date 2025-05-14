#pragma once

#include <Windows.h>
#include <string>

class Window {
public:
    Window(HINSTANCE hInstance, const std::wstring& title, int32_t width, int32_t height);
    ~Window();

    HWND getHwnd() const { return hwnd_; }

    bool processMessage();

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND hwnd_;
    HINSTANCE hInstance_;
};
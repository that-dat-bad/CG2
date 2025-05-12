#include "Utils.h"
#include <format>
#include <strsafe.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

// 文字列変換 (std::string -> std::wstring)
std::wstring ConvertString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }
    int32_t size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size_needed == 0) {
        return std::wstring();
    }
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size_needed);
    return result;
}

// 文字列変換 (std::wstring -> std::string)
std::string ConvertString(const std::wstring& str) {
    if (str.empty()) {
        return std::string();
    }
    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) {
        return std::string();
    }
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;
}

// ログ出力
void Log(const std::string& message) {
#ifdef _DEBUG
    OutputDebugStringA(message.c_str());
#endif
}

// クラッシュレポート
void __stdcall ExportDump(struct _EXCEPTION_POINTERS* apExceptionInfo) {
    HANDLE hFile = CreateFile(L"crashdump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        SYSTEMTIME time;
        GetLocalTime(&time);
        wchar_t filePath[MAX_PATH] = { 0 };
        CreateDirectory(L"./Dump", nullptr);
        StringCchPrintfW(filePath, MAX_PATH, L"./Dump/%04d-%02d-%02d-%02d-%02d.dmp",
            time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = apExceptionInfo;
        mdei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, NULL, NULL);
        CloseHandle(hFile);
    }
    exit(0);
}

// ウィンドウプロシージャ
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return true;
    }

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
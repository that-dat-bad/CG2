#pragma once

#include <Windows.h>
#include <string>

// 文字列変換
std::wstring ConvertString(const std::string& str);
std::string ConvertString(const std::wstring& str);

// ログ出力
void Log(const std::string& message);

// クラッシュレポート
void __stdcall ExportDump(struct _EXCEPTION_POINTERS* apExceptionInfo);

// ウィンドウプロシージャ
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
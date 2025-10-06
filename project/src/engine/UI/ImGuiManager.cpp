#include "ImGuiManager.h"
#include <../external/imgui/imgui.h>
#include <../external/imgui/imgui_impl_win32.h>
#include <../external/imgui/imgui_impl_dx12.h>
#include "../base/WinApp.h"

class Winapp;

void ImGuiManager::Initialize(WinApp* winApp)
{
	//コンテキストの生成
	ImGui::CreateContext();

	//スタイルを設定
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(winApp->GetHwnd());
}
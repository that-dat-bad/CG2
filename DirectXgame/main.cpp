#include<Windows.h>
#include<cstdint>
#include<string>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#include<format>
#include<comdef.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")


//ログ用関数
void Log(const std::string& message) {
	//出力ウィンドウにメッセージを表示
	OutputDebugStringA(message.c_str());
}



//DXGIファクトリーのポインタ変数を定義
IDXGIFactory* dxgiFactory = nullptr;
//DXGIファクトリーの生成
HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
//DXGIファクトリーの成否判定
assert(SUCCEEDED(hr));

//アダプタ用の変数
IDXGIAdapter4* useAdapter = nullptr;
// 良い順にアダプタを頼む
for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
	DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
	DXGI_ERROR_NOT_FOUND; ++i) {

	// アダプターの情報を取得する
	DXGI_ADAPTER_DESC3 adapterDesc{};
	hr useAdapter->GetDesc3(&adapterDesc);
	assert(SUCCEEDED(hr));
	// ソフトウェアアダプタでなければ採用!
	if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {

		// 採用したアダプタの情報をログに出力。wstringの方なので注意
		Log(std::format(L"Use Adapater:{}\n", adapterDesc.Description));
		break;
	}

	useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
}

// 適切なアダプタが見つからなかったので起動できない
assert(useAdapter != nullptr);


//D3D12Deviceの生成
ID3D12Device* device = nullptr;




// クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg,
	WPARAM wParam, LPARAM lParam) {
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		// ウィンドウが破棄されたとき
	case WM_DESTROY:
		//OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	WNDCLASS wc{};

	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;

	//ウィンドウクラス名
	wc.lpszClassName = L"CG2WindowClass";

	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);

	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass(&wc);


	// クライアント領域の矩形を定義
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };

	// クライアント領域を元に実際のウィンドウサイズに調整
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウの作成
	HWND hwnd = CreateWindow(
		wc.lpszClassName, //利用するクラス名
		L"CG2", //タイトルバーの文字
		WS_OVERLAPPEDWINDOW, //よく見るウィンドウスタイル
		CW_USEDEFAULT, //表示X座標(windowsに任せる)
		CW_USEDEFAULT, //表示Y座標(windowsに任せる)
		wrc.right - wrc.left, //ウィンドウ横幅
		wrc.bottom - wrc.top, //ウィンドウ縦幅
		nullptr, //親ウィンドウハンドル
		nullptr, //メニューハンドル
		wc.hInstance, //インスタンスハンドル
		nullptr); //オプション

	//ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	MSG msg{};
	//windowのxが押されるまでループ
	while (msg.message != WM_QUIT) {
		//windowにメッセージが来てたら最優先で処理させる
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//ゲームの処理
		}
	}
	//出力ウィンドウへの文字出力
	OutputDebugStringA("Hello, DirectX!\n");

	return 0;
}
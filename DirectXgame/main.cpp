#include<Windows.h>
#include<cstdint>
#include<string>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#include<format>
#include<strsafe.h>

#include <dxcapi.h>


//debug用のヘッダ
#include<DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

#include<dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib,"dxcompiler.lib")

//ログ用関数
void Log(const std::string& message) {
	//出力ウィンドウにメッセージを表示
	OutputDebugStringA(message.c_str());
}

//Vector4の定義
struct Vector4
{
	float x, y, z, w;
};

//string<->wstring変換
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

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

//===================crashHandler=========================

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {

	//時刻を取得して時刻を名前に入れたファイルを作成
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dump", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dump/%04d-%02d-%02d-%02d-%02d.dmp",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	//processId(このexeのID)とクラッシュの発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	//設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;

	//Dumpを出力、MiniDumpNomalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	//他に関連付けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する。
	return EXCEPTION_EXECUTE_HANDLER;
}

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





IDxcBlob* CompileShader(
	//コンパイルするシェーダーのファイルパス
	const std::wstring& filePath,
	//コンパイルに使用するプロファイル
	const wchar_t* profile,
	//初期化で生成したもの3つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler) {

	//1.hlslファイルを読み込む
	//これからシェーダーをコンパイルする旨をログに出力
	Log(ConvertString(std::format(L"Begin CompileShader, Path :{}, profile : {}\n", filePath, profile)));

	//hlslファイルを読み込む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

	//読めなかったら止める
	assert(SUCCEEDED(hr));

	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer{};
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer(); //バッファのポインタ
	shaderSourceBuffer.Size = shaderSource->GetBufferSize(); //バッファのサイズ
	shaderSourceBuffer.Encoding = DXC_CP_UTF8; //エンコーディングの種類(utf-8)

	//2.コンパする
	LPCWSTR arguments[] = {
		filePath.c_str(), //コンパイルするファイルのパス
		L"-E", L"main", //エントリーポイント
		L"-T", profile, //プロファイル
		L"-Zi", //デバッグ情報を出力
		L"-Od", //最適化をしない
		L"-Zpr",//メモリは行優先
	};
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer, //コンパイルするシェーダーの情報
		arguments, //コンパイルに使用する引数
		_countof(arguments), //引数の数
		includeHandler, //インクルードの諸々
		IID_PPV_ARGS(&shaderResult) //結果を格納するポインタ
	);
	//dxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	//3.コンパイル結果を取得する
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Log(shaderError->GetStringPointer());
		//警告・エラーダメ絶対
		assert(false);
	}

	//4.コンパイル結果を受け取って返す
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);

	//コンパイル結果を受け取れなかったら止める
	assert(SUCCEEDED(hr));

	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succeeded, Path :{}, profile : {}\n", filePath, profile)));
	//解放
	shaderSource->Release();
	shaderResult->Release();

	//実行用のバイナリを返す
	return shaderBlob;
};



//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	SetUnhandledExceptionFilter(ExportDump);


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
	//故意のクラッシュコード
	//uint32_t* p = nullptr;
	//*p = 100;


#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		//デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();

		//GPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif // DEBUG


	//===================DX12=========================

	//DXGIファクトリーのポインタ変数を定義
	IDXGIFactory6* dxgiFactory = nullptr;
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
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		// ソフトウェアアダプタでなければ採用!
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプタの情報をログに出力。wstringの方なので注意
			Log(ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
			break;
		}

		useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
	}

	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);


	//D3D12Deviceの生成
	ID3D12Device* device = nullptr;

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};

	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		// 採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		//指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr)) {
			// 生成できたのでログ出力を行ってループを抜ける
			Log(std::format("FeatureLevel: {}\n", featureLevelStrings[1]));
			break;
		}
	}

	// デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");// 初期化完了のログをだす

#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		//マジヤバエラー
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

		//エラー
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		//警告
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		//抑制するメッセージ
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = {
			D3D12_MESSAGE_SEVERITY_INFO
		};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		//指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);


		//解放
		infoQueue->Release();
	}


#endif // _DEBUG




	//コマンドキューを生成
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));

	//コマンドキューの生成失敗
	assert(SUCCEEDED(hr));


	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	//Fencenosignalを持つためのイベントを作成
	HANDLE fenceEvent = CreateEvent(NULL, false, false, NULL);
	assert(fenceEvent != nullptr);

	//dxCompilerの初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));




	//スワップチェーンの生成
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth; //ウィンドウの横幅
	swapChainDesc.Height = kClientHeight; //ウィンドウの縦幅
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //色形式
	swapChainDesc.SampleDesc.Count = 1; //マルチサンプリングの数
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //バッファの使用方法
	swapChainDesc.BufferCount = 2; //バッファの数
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //スワップチェインの効果

	//コマンドキュー、ウィンドウハンドル、スワップチェインの情報を渡す
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue, //コマンドキュー
		hwnd, //ウィンドウハンドル
		&swapChainDesc, //スワップチェインの情報
		nullptr, //フルスクリーンの情報
		nullptr, //コマンドキューの情報
		reinterpret_cast<IDXGISwapChain1**>(&swapChain) //スワップチェインのポインタ
	);


	//ディスクリプタヒープの生成
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビュー
	rtvDescriptorHeapDesc.NumDescriptors = 2; //ダブルバッファ用
	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));


	//ディスクリプタヒープの生成失敗
	assert(SUCCEEDED(hr));


	//スワップチェーンからリソースを取得
	ID3D12Resource* swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));

	//取得失敗
	assert(SUCCEEDED(hr));

	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));

	//取得失敗
	assert(SUCCEEDED(hr));


	//レンダーターゲットビュー(RTV)の設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //出力結果をSRGBに変換して読み込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; //2Dテクスチャとして書き込む


	//ディスクリプタの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	//RTVを2つ作るのでディスクリプタも2つ
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]{};

	//1つ目のRTVを作成
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);

	//2つ目のRTVを作成
	rtvHandles[1] = { rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);


	//コマンドアロケータを生成
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

	//コマンドアロケータの生成失敗
	assert(SUCCEEDED(hr));


	//コマンドリストを生成
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator, nullptr, IID_PPV_ARGS(&commandList));


	//コマンドリストの生成失敗
	assert(SUCCEEDED(hr));


	//====================PSO=========================

	//RootSignatureの生成

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; //入力レイアウトを許可

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	//RootSignatureのシリアライズ失敗
	if (FAILED(hr))
	{
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	//バイナリをもとに生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	//RootSignatureの生成失敗
	assert(SUCCEEDED(hr));


	//InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION"; //セマンティクス名
	inputElementDescs[0].SemanticIndex = 0; //セマンティクスのインデックス
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; //データ形式
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; //オフセット
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs; //セマンティクスの情報
	inputLayoutDesc.NumElements = _countof(inputElementDescs); //セマンティクスの数


	//BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//ポリゴンの描画方法
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//shaderのコンパイル
	IDxcBlob* vertexShaderBlob = CompileShader(
		L"Object3D.VS.hlsl", //コンパイルするシェーダーのファイルパス
		L"vs_6_0", //コンパイルに使用するプロファイル
		dxcUtils, dxcCompiler, includeHandler);

	IDxcBlob* pixelShaderBlob = CompileShader(
		L"Object3D.PS.hlsl", //コンパイルするシェーダーのファイルパス
		L"ps_6_0", //コンパイルに使用するプロファイル
		dxcUtils, dxcCompiler, includeHandler);

	//PSOの生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	graphicPipelineStateDesc.pRootSignature = rootSignature; //ルートシグネチャ
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc; //入力レイアウト
	graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() }; //頂点シェーダー
	graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() }; //ピクセルシェーダー
	graphicPipelineStateDesc.BlendState = blendDesc; //ブレンドステート
	graphicPipelineStateDesc.RasterizerState = rasterizerDesc; //ラスタライザーステート
	//書き込むRTVの情報
	graphicPipelineStateDesc.NumRenderTargets = 1; //RTVの数
	graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //RTVの形式
	//利用するトポロジのタイプ
	graphicPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; //トポロジのタイプ
	//どのように画面に色を打ち込むか
	graphicPipelineStateDesc.SampleDesc.Count = 1;
	graphicPipelineStateDesc.SampleMask = UINT_MAX;

	//実際に生成
	ID3D12PipelineState* graphicPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicPipelineState));
	//PSOの生成失敗
	assert(SUCCEEDED(hr));

	//頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; //アップロード用ヒープ
	//頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};

	//バッファリソース
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; //バッファリソース
	vertexResourceDesc.Width = sizeof(Vector4) * 3;//バッファのサイズ

	//バッファはすべて1
	vertexResourceDesc.Height = 1; 
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1; 

	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//実際に頂点リソースを生成
	ID3D12Resource* vertexResource = nullptr;
	hr = device->CreateCommittedResource(
		&uploadHeapProperties, //ヒープの設定
		D3D12_HEAP_FLAG_NONE, //ヒープのフラグ
		&vertexResourceDesc, //リソースの設定
		D3D12_RESOURCE_STATE_GENERIC_READ, //リソースの状態
		nullptr, //クリエイトの設定
		IID_PPV_ARGS(&vertexResource) //リソースのポインタ
	);
	//頂点リソースの生成失敗
	assert(SUCCEEDED(hr));

	//頂点リソースにデータを書き込む
	Vector4* vertexData = nullptr;

	//書き込むためのポインタを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	vertexData[0] = { -0.5f, -0.5f, 0.0f, 1.0f }; //頂点1
	vertexData[1] = { 0.0f, 0.5f, 0.0f, 1.0f }; //頂点2
	vertexData[2] = { 0.5f, -0.5f, 0.0f, 1.0f }; //頂点3

	//頂点バッファビューを生成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	//リソースの先頭アドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); //リソースの先頭アドレス

	//使用するリソースのサイズ
	vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;

	//1つの頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(Vector4);

		//===================message=========================
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



			//これから書き込むバックバッファのインデックスを取得
			UINT backbufferIndex = swapChain->GetCurrentBackBufferIndex();

			//描画先のRTVを設定
			commandList->OMSetRenderTargets(1, &rtvHandles[backbufferIndex], false, nullptr);


			//TransitionBarrierの設定
			D3D12_RESOURCE_BARRIER barrier{};

			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; //トランジションバリア

			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; //フラグなし

			barrier.Transition.pResource = swapChainResources[backbufferIndex]; //対象のリソース

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; //遷移前の状態

			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; //遷移後の状態

			commandList->ResourceBarrier(1, &barrier); //バリアを設定


			//指定した色で画面をクリア
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
			commandList->ClearRenderTargetView(rtvHandles[backbufferIndex], clearColor, 0, nullptr);


			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; //遷移前の状態
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT; //遷移後の状態
			commandList->ResourceBarrier(1, &barrier); //バリアを設定

			hr = commandList->Close();
			//コマンドリストのクローズ失敗
			assert(SUCCEEDED(hr));

			//GPUにコマンドリストの実行を行わせる
			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);

			//GPUとOSに画面の交換を行うように通知する
			swapChain->Present(1, 0);


			//Fenceの値を更新
			fenceValue++;

			//GPUがここまでたどり着いたらFenceの値を代入
			commandQueue->Signal(fence, fenceValue);

			if (fence->GetCompletedValue() < fenceValue)
			{
				// 指定したSignalにたどりついていないので、たどり着くまで待つよう
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				// イベント待つ
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			//次のフレーム用のコマンドリストを準備
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));

			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));

			//ビューポート
			D3D12_VIEWPORT viewport{};

			//クライアント領域のサイズと一緒にして全体に表示
			viewport.Width = kClientWidth;
			viewport.Height = kClientHeight;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			//シザー矩形
			D3D12_RECT scissorRect{};
			//クライアント領域のサイズと一緒にして全体に表示
			scissorRect.left = 0;
			scissorRect.right = kClientWidth;
			scissorRect.top = 0;
			scissorRect.bottom = kClientHeight;

			//コマンドを積む
			commandList->RSSetViewports(1, &viewport); //ビューポートの設定
			commandList->RSSetScissorRects(1, &scissorRect); //シザー矩形の設定
			//ルートシグネチャを設定
			commandList->SetGraphicsRootSignature(rootSignature);
			commandList->SetPipelineState(graphicPipelineState); //PSOの設定
			//頂点バッファビューを設定
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			//形状を設定
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//描画
			commandList->DrawInstanced(3, 1, 0, 0); //インスタンス数、頂点数、インデックス、オフセット
		}
	}
	//出力ウィンドウへの文字出力
	OutputDebugStringA("Hello, DirectX!\n");
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	//解放処理
	CloseHandle(fenceEvent);
	fence->Release();
	rtvDescriptorHeap->Release();
	swapChainResources[0]->Release();
	swapChainResources[1]->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	device->Release();
	useAdapter->Release();
	dxgiFactory->Release();
#ifdef _DEBUG
	debugController->Release();
#endif // _DEBUG
	vertexResource->Release();
	graphicPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob) 
	{
		errorBlob->Release();
	}
	rootSignature->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();


	CloseWindow(hwnd);

	return 0;
}
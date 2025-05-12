#include "Engine.h"
#include "Utils.h"
#include <cassert>
#include <format>
#include <strsafe.h>
#include <dxcapi.h>

// クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

Engine::Engine() :
    hwnd_(nullptr),
    dxgiFactory_(nullptr),
    useAdapter_(nullptr),
    device_(nullptr),
    commandQueue_(nullptr),
    swapChain_(nullptr),
    rtvDescriptorHeap_(nullptr),
    commandAllocator_(nullptr),
    commandList_(nullptr),
    fence_(nullptr),
    fenceValue_(0),
    fenceEvent_(nullptr),
    dxcUtils_(nullptr),
    dxcCompiler_(nullptr),
    includeHandler_(nullptr) {
}

Engine::~Engine() {
}

bool Engine::Initialize(HINSTANCE hInstance, int nCmdShow) {
    // COMの初期化
    CoInitializeEx(0, COINIT_MULTITHREADED);

    // クラッシュハンドラーの設定
    SetUnhandledExceptionFilter(ExportDump);

    // ウィンドウの作成
    WNDCLASS wc{};
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"CG2WindowClass";
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    RECT wrc = { 0, 0, kClientWidth, kClientHeight };
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    hwnd_ = CreateWindow(
        wc.lpszClassName, L"CG2", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wrc.right - wrc.left, wrc.bottom - wrc.top,
        nullptr, nullptr, wc.hInstance, nullptr);

    ShowWindow(hwnd_, SW_SHOW);

#ifdef _DEBUG
    // デバッグレイヤーの有効化
    ID3D12Debug1* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(TRUE);
    }
#endif

    // DirectX12の初期化
    if (!InitDirectX12()) {
        return false;
    }

    // ImGuiの初期化
    if (!InitImGui()) {
        return false;
    }

    // Rendererの初期化
    if (!renderer_.Initialize(device_, hwnd_)) {
        return false;
    }

    return true;
}

void Engine::Run() {
    MSG msg{};

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // ゲームの処理
            Update();
            Draw();
        }
    }
}

void Engine::Finalize() {
    // ImGuiの終了処理
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // DirectX12の終了処理
    FinalizeDirectX12();

    CloseWindow(hwnd_);

    // COMの終了
    CoUninitialize();
}

bool Engine::InitDirectX12() {
    // DXGIファクトリーの生成
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
    if (FAILED(hr)) {
        return false;
    }

    // アダプターの選択
    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) !=
        DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter_->GetDesc3(&adapterDesc);
        if (SUCCEEDED(hr)) {
            if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
                Log(ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
                break;
            }
        }
        useAdapter_ = nullptr;
    }
    if (useAdapter_ == nullptr) {
        return false;
    }

    // デバイスの生成
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };

    const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(useAdapter_, featureLevels[i], IID_PPV_ARGS(&device_));
        if (SUCCEEDED(hr)) {
            Log(std::format("FeatureLevel: {}\n", featureLevelStrings[1]));
            break;
        }
    }
    if (device_ == nullptr) {
        return false;
    }
    Log("Complete create D3D12Device!!!\n");

#ifdef _DEBUG
    // デバッグ設定
    ID3D12InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };
        D3D12_MESSAGE_SEVERITY severities[] = {
            D3D12_MESSAGE_SEVERITY_INFO
        };
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;
        infoQueue->PushStorageFilter(&filter);
        infoQueue->Release();
    }
#endif

    // コマンドキューの生成
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
    if (FAILED(hr)) {
        return false;
    }

    // スワップチェーンの生成
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = kClientWidth;
    swapChainDesc.Height = kClientHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_, hwnd_, &swapChainDesc, nullptr, nullptr,
        reinterpret_cast<IDXGISwapChain1**>(&swapChain_));
    if (FAILED(hr)) {
        return false;
    }

    // RTV用ディスクリプタヒープの生成
    rtvDescriptorHeap_ = CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
    if (rtvDescriptorHeap_ == nullptr) {
        return false;
    }

    // スワップチェーンからリソースを取得
    hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
    if (FAILED(hr)) {
        return false;
    }
    hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
    if (FAILED(hr)) {
        return false;
    }

    // RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]{};
    rtvHandles[0] = rtvStartHandle;
    device_->CreateRenderTargetView(swapChainResources_[0], &rtvDesc, rtvHandles[0]);
    rtvHandles[1] = { rtvHandles[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
    device_->CreateRenderTargetView(swapChainResources_[1], &rtvDesc, rtvHandles[1]);

    // コマンドアロケータ、コマンドリストの生成
    hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
    if (FAILED(hr)) {
        return false;
    }
    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_, nullptr, IID_PPV_ARGS(&commandList_));
    if (FAILED(hr)) {
        return false;
    }

    // Fenceの生成
    hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    if (FAILED(hr)) {
        return false;
    }
    fenceEvent_ = CreateEvent(NULL, false, false, NULL);
    if (fenceEvent_ == nullptr) {
        return false;
    }

    // DXCの初期化
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
    if (FAILED(hr)) {
        return false;
    }
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
    if (FAILED(hr)) {
        return false;
    }
    hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool Engine::InitImGui() {
    // ImGuiの初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX12_Init(device_, 2,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        srvDescriptorHeap_,
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
        srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart()
    );
    return true;
}

void Engine::FinalizeDirectX12() {
#ifdef _DEBUG
    // デバッグレイヤーの終了処理
    ID3D12Debug1* debugController = nullptr;
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->Release();
    }
#endif

    // 各種リソースの解放
    CloseHandle(fenceEvent_);
    fence_->Release();
    rtvDescriptorHeap_->Release();
    swapChainResources_[0]->Release();
    swapChainResources_[1]->Release();
    swapChain_->Release();
    commandList_->Release();
    commandAllocator_->Release();
    commandQueue_->Release();
    device_->Release();
    useAdapter_->Release();
    dxgiFactory_->Release();
    dxcUtils_->Release();
    dxcCompiler_->Release();
    includeHandler_->Release();
}

void Engine::Update() {
    // シーンの更新
    scene_.Update();

    // ゲームの更新処理
}

void Engine::Draw() {
    // 描画処理
    UINT backbufferIndex = swapChain_->GetCurrentBackBufferIndex();

    // ResourceBarrier: Present → RenderTarget
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources_[backbufferIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrier);

    // 描画先のRTVを設定
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();  // 仮
    commandList_->OMSetRenderTargets(1, &rtvHandles[backbufferIndex], false, &dsvHandle);

    // 画面をクリア
    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList_->ClearRenderTargetView(rtvHandles[backbufferIndex], clearColor, 0, nullptr);
    commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 仮

    // ImGuiの描画
    ImGui::Render();
    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_ };  // 仮
    commandList_->SetDescriptorHeaps(1, descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_);

    // シーンの描画
    scene_.Draw();

    // 描画コマンドの記録
    renderer_.Draw(commandList_);

    // Present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList_->ResourceBarrier(1, &barrier);

    // コマンドリストのクローズと実行
    HRESULT hr = commandList_->Close();
    assert(SUCCEEDED(hr));
    ID3D12CommandList* commandLists[] = { commandList_ };
    commandQueue_->ExecuteCommandLists(1, commandLists);

    swapChain_->Present(1, 0);

    // Fenceの更新と待機
    fenceValue_++;
    commandQueue_->Signal(fence_, fenceValue_);
    if (fence_->GetCompletedValue() < fenceValue_) {
        commandQueue_->Signal(fence_, fenceValue_);
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }

    // 次のフレーム用のコマンドリストを準備
    hr = commandAllocator_->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList_->Reset(commandAllocator_, nullptr);
    assert(SUCCEEDED(hr));
}
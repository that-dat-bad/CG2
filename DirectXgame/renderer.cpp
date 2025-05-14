#include "renderer.h"
#include <cassert>

Renderer::Renderer(HWND hwnd, int32_t width, int32_t height)
    : hwnd_(hwnd), clientWidth_(width), clientHeight_(height) {
    initializeDirectX12();
}

Renderer::~Renderer() {
    // 解放処理
    CloseHandle(fenceEvent_);
    fence_->Release();
    rtvDescriptorHeap_->Release();
    dsvDescriptorHeap_->Release();
    depthStencilBuffer_->Release();
    swapChainResources_[0]->Release();
    swapChainResources_[1]->Release();
    swapChain_->Release();
    commandList_->Release();
    commandAllocator_->Release();
    commandQueue_->Release();
    device_->Release();
    useAdapter_->Release();
    dxgiFactory_->Release();
}

bool Renderer::initializeDirectX12() {
    // 1. Factoryの生成
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
    if (FAILED(hr)) {
        return false;
    }

    // 2. アダプターの列挙
    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) !=
        DXGI_ERROR_NOT_FOUND; ++i) {
        // ... (省略: 高パフォーマンスアダプターを探す処理)
    }
    if (!useAdapter_) {
        return false;
    }

    // 3. デバイスの生成
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };
    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(useAdapter_, featureLevels[i], IID_PPV_ARGS(&device_));
        if (SUCCEEDED(hr)) {
            break;
        }
    }
    if (!device_) {
        return false;
    }

    // InfoQueueの設定 (DebugLayer有効時のみ)
#ifdef _DEBUG
    ID3D12InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        // ... (省略: InfoQueueの設定)
        infoQueue->Release();
    }
#endif

    // 4. コマンドキューの生成
    if (!createCommandQueue()) {
        return false;
    }

    // 5. スワップチェインの生成
    if (!createSwapChain()) {
        return false;
    }

    // 6. レンダーターゲットビュー (RTV) の生成
    if (!createRenderTargetView()) {
        return false;
    }

    // 7. 深度ステンシルビュー (DSV) の生成
    if (!createDepthStencilView()) {
        return false;
    }

    // 8. コマンドアロケータとコマンドリストの生成
    if (!createCommandAllocatorAndList()) {
        return false;
    }

    // 9. FenceとEventの生成
    if (!createFenceAndEvent()) {
        return false;
    }

    return true;
}

bool Renderer::createCommandQueue() {
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    HRESULT hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
    return SUCCEEDED(hr);
}

bool Renderer::createSwapChain() {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = clientWidth_;
    swapChainDesc.Height = clientHeight_;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_,
        hwnd_,
        &swapChainDesc,
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(&swapChain_)
    );
    return SUCCEEDED(hr);
}

bool Renderer::createRenderTargetView() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescriptorHeapDesc.NumDescriptors = 2;
    rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDescriptorHeapDesc.NodeMask = 0;
    HRESULT hr = device_->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap_));
    if (FAILED(hr)) {
        return false;
    }

    for (UINT i = 0; i < 2; ++i) {
        hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
        if (FAILED(hr)) {
            return false;
        }
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += i * device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device_->CreateRenderTargetView(swapChainResources_[i], &rtvDesc, rtvHandle);
    }
    return true;
}

bool Renderer::createDepthStencilView() {
    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc{};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvDescriptorHeapDesc.NodeMask = 0;
    HRESULT hr = device_->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap_));
    if (FAILED(hr)) {
        return false;
    }

    D3D12_RESOURCE_DESC depthStencilDesc{};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Width = clientWidth_;
    depthStencilDesc.Height = clientHeight_;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES depthStencilHeapProperties{};
    depthStencilHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    depthStencilHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    depthStencilHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    depthStencilHeapProperties.CreationNodeMask = 0;
    depthStencilHeapProperties.VisibleNodeMask = 0;

    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.DepthStencil.Stencil = 0;

    hr = device_->CreateCommittedResource(
        &depthStencilHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&depthStencilBuffer_)
    );
    if (FAILED(hr)) {
        return false;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    dsvHandle_ = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    device_->CreateDepthStencilView(depthStencilBuffer_, &dsvDesc, dsvHandle_);

    return true;
}

bool Renderer::createCommandAllocatorAndList() {
    HRESULT hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_, nullptr, IID_PPV_ARGS(&commandList_));
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool Renderer::createFenceAndEvent() {
    HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    if (FAILED(hr)) {
        return false;
    }

    fenceEvent_ = CreateEvent(NULL, false, false, NULL);
    return fenceEvent_ != nullptr;
}

void Renderer::render() {
    // 描画処理はまだ実装しない
}
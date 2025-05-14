#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include "math/math_utils.h" // または "math/geometry.h"
#include "window/window.h"

class Renderer {
public:
    Renderer(HWND hwnd, int32_t width, int32_t height);
    ~Renderer();

    void render(); // 描画処理
    ID3D12Device* getDevice() const { return device_; } // Deviceへのアクセサ

private:
    bool initializeDirectX12();
    bool createSwapChain();
    bool createCommandQueue();
    bool createRenderTargetView();
    bool createDepthStencilView();
    bool createCommandAllocatorAndList();
    bool createFenceAndEvent();

private:
    // DirectX 12関連オブジェクト
    IDXGIFactory6* dxgiFactory_ = nullptr;
    IDXGIAdapter4* useAdapter_ = nullptr;
    ID3D12Device* device_ = nullptr;
    ID3D12CommandQueue* commandQueue_ = nullptr;
    IDXGISwapChain4* swapChain_ = nullptr;
    ID3D12DescriptorHeap* rtvDescriptorHeap_ = nullptr;
    ID3D12Resource* swapChainResources_[2] = { nullptr };
    ID3D12CommandAllocator* commandAllocator_ = nullptr;
    ID3D12GraphicsCommandList* commandList_ = nullptr;
    ID3D12Fence* fence_ = nullptr;
    HANDLE fenceEvent_ = nullptr;
    uint64_t fenceValue_ = 0;
    ID3D12DescriptorHeap* dsvDescriptorHeap_ = nullptr;
    ID3D12Resource* depthStencilBuffer_ = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_;

    int32_t clientWidth_;
    int32_t clientHeight_;
    HWND hwnd_;
};
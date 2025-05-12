#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include "Renderer.h"
#include "Scene.h"
#include "MyMath.h"
#include "External/ImGui.h"
#include "External/DirectXTex.h"
#include "Utils.h"

class Engine {
public:
    Engine();
    ~Engine();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    void Run();
    void Finalize();

private:
    // ウィンドウハンドル
    HWND hwnd_;

    // DirectX12関連
    IDXGIFactory6* dxgiFactory_;
    IDXGIAdapter4* useAdapter_;
    ID3D12Device* device_;
    ID3D12CommandQueue* commandQueue_;
    IDXGISwapChain4* swapChain_;
    ID3D12DescriptorHeap* rtvDescriptorHeap_;
    ID3D12Resource* swapChainResources_[2];
    ID3D12CommandAllocator* commandAllocator_;
    ID3D12GraphicsCommandList* commandList_;
    ID3D12Fence* fence_;
    uint64_t fenceValue_;
    HANDLE fenceEvent_;

    // DirectX Compiler
    IDxcUtils* dxcUtils_;
    IDxcCompiler3* dxcCompiler_;
    IDxcIncludeHandler* includeHandler_;

    // 描画関連
    Renderer renderer_;

    // シーン管理
    Scene scene_;
};
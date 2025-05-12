#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include "MyMath.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(ID3D12Device* device, HWND hwnd);
    void Draw(ID3D12GraphicsCommandList* commandList);

private:
    ID3D12Device* device_;
    HWND hwnd_;

    // DirectX Compiler
    IDxcUtils* dxcUtils_;
    IDxcCompiler3* dxcCompiler_;
    IDxcIncludeHandler* includeHandler_;

    // PSO
    ID3D12PipelineState* pipelineState_;

    // ルートシグネチャ
    ID3D12RootSignature* rootSignature_;

    // 頂点バッファ関連
    ID3D12Resource* vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;

    // インデックスバッファ関連
    ID3D12Resource* indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;

    // 定数バッファ関連
    ID3D12Resource* constantBuffer_;
    D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc_;
    struct ConstantBuffer {
        MyMath::Matrix4x4 worldViewProjection;
    };
    ConstantBuffer constantBufferData_;
    UINT8* constantBufferMappedAddress_;
};
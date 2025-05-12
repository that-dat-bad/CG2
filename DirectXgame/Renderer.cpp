#include "Renderer.h"
#include "Utils.h"
#include <cassert>
#include <d3dcompiler.h>

Renderer::Renderer() :
    device_(nullptr),
    hwnd_(nullptr),
    dxcUtils_(nullptr),
    dxcCompiler_(nullptr),
    includeHandler_(nullptr),
    pipelineState_(nullptr),
    rootSignature_(nullptr),
    vertexBuffer_(nullptr),
    indexBuffer_(nullptr),
    constantBuffer_(nullptr),
    constantBufferMappedAddress_(nullptr) {
}

Renderer::~Renderer() {
    if (constantBufferMappedAddress_) {
        constantBuffer_->Unmap(0, nullptr);
        constantBufferMappedAddress_ = nullptr;
    }
    if (constantBuffer_) constantBuffer_->Release();
    if (indexBuffer_) indexBuffer_->Release();
    if (vertexBuffer_) vertexBuffer_->Release();
    if (rootSignature_) rootSignature_->Release();
    if (pipelineState_) pipelineState_->Release();
    if (dxcUtils_) dxcUtils_->Release();
    if (dxcCompiler_) dxcCompiler_->Release();
    if (includeHandler_) includeHandler_->Release();
}

bool Renderer::Initialize(ID3D12Device* device, HWND hwnd) {
    device_ = device;
    hwnd_ = hwnd;

    HRESULT hr;

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

    // ルートシグネチャの作成
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_ROOT_PARAMETER rootParameters[1]{};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = _countof(rootParameters);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].MipLODBias = 0.0f;
    staticSamplers[0].MaxAnisotropy = 0;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSamplers[0].MinLOD = 0.0f;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].RegisterSpace = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
        &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            Log(static_cast<char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        return false;
    }
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    signatureBlob->Release();
    if (FAILED(hr)) {
        return false;
    }

    // PSOの作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature_;
    psoDesc.VS = LoadShader(L"shader.hlsl", "VSMain", "vs_6_0");
    psoDesc.PS = LoadShader(L"shader.hlsl", "PSMain", "ps_6_0");
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.InputLayout = { Vertex::inputElements, _countof(Vertex::inputElements) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
    if (FAILED(hr)) {
        return false;
    }

    // 頂点バッファの作成
    Vertex vertices[] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 左下
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // 右下
        {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},   // 上
    };

    D3D12_HEAP_PROPERTIES heapProp{};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(vertices);
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    hr = device_->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&vertexBuffer_));
    if (FAILED(hr)) {
        return false;
    }

    Vertex* pVertexDataBegin = nullptr;
    hr = vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(hr)) {
        return false;
    }
    memcpy(pVertexDataBegin, vertices, sizeof(vertices));
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = sizeof(Vertex);
    vertexBufferView_.SizeInBytes = sizeof(vertices);

    // インデックスバッファの作成
    uint16_t indices[] = { 0, 1, 2 };

    resDesc.Width = sizeof(indices);
    hr = device_->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&indexBuffer_));
    if (FAILED(hr)) {
        return false;
    }

    uint16_t* pIndexDataBegin = nullptr;
    hr = indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&pIndexDataBegin));
    if (FAILED(hr)) {
        return false;
    }
    memcpy(pIndexDataBegin, indices, sizeof(indices));
    indexBuffer_->Unmap(0, nullptr);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.Format = DXGI_FORMAT_R16_UINT;
    indexBufferView_.SizeInBytes = sizeof(indices);

    // 定数バッファの作成
    resDesc.Width = (sizeof(ConstantBuffer) + 255) & ~255; // 256バイトアラインメント
    hr = device_->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&constantBuffer_));
    if (FAILED(hr)) {
        return false;
    }

    hr = constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constantBufferMappedAddress_));
    if (FAILED(hr)) {
        return false;
    }

    constantBufferViewDesc_.BufferLocation = constantBuffer_->GetGPUVirtualAddress();
    constantBufferViewDesc_.SizeInBytes = sizeof(ConstantBuffer);

    return true;
}

void Renderer::Draw(ID3D12GraphicsCommandList* commandList) {
    // 定数バッファの更新
    MyMath::Matrix4x4 worldMatrix = MyMath::MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
    MyMath::Matrix4x4 viewMatrix = MyMath::MakeIdentity4x4(); // 仮
    MyMath::Matrix4x4 projectionMatrix = MyMath::MakePerspectiveFovLH(XMConvertToRadians(60.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    constantBufferData_.worldViewProjection = MyMath::Multiply(worldMatrix, MyMath::Multiply(viewMatrix, projectionMatrix));
    memcpy(constantBufferMappedAddress_, &constantBufferData_, sizeof(constantBufferData_));

    // パイプラインステートとルートシグネチャの設定
    commandList->SetPipelineState(pipelineState_);
    commandList->SetGraphicsRootSignature(rootSignature_);

    // 頂点バッファとインデックスバッファの設定
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);

    // プリミティブトポロジーの設定
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 定数バッファのビューを設定
    commandList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());

    // 描画コマンドの記録
    commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
}

// シェーダーをロードしてコンパイルする関数
D3D12_SHADER_BYTECODE Renderer::LoadShader(const std::wstring& filename, const char* entrypoint, const char* target) {
    D3D12_SHADER_BYTECODE shaderBytecode = {};
    ComPtr<IDxcBlob> shaderBlob;
    ComPtr<IDxcBlob> errorBlob;
    HRESULT hr = dxcCompiler_->Compile(
        ConvertString(filename).c_str(),
        ConvertString(filename).c_str(),
        entrypoint,
        target,
        nullptr, 0,
        nullptr, 0,
        includeHandler_.Get(),
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            Log(static_cast<char*>(errorBlob->GetBufferPointer()));
        }
        return shaderBytecode;
    }

    shaderBytecode.pShaderBytecode = shaderBlob->GetBufferPointer();
    shaderBytecode.BytecodeLength = shaderBlob->GetBufferSize();

    return shaderBytecode;
}
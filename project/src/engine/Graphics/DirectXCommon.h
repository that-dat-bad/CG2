#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include"./engine/base/WinApp.h"
#include <array>
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_dx12.h"
#include "../external/imgui/imgui_impl_win32.h"

class DirectXCommon
{
public:

	static const uint32_t kRtvHeapDescriptorNum_ = 2; // ダブルバッファ用
	static const uint32_t kSrvHeapDescriptorNum_ = 100; // SRVの最大数
	static const uint32_t kDsvHeapDescriptorNum_ = 1; // 深度バッファ用
	static const uint32_t kSwapChainBufferCount_ = 2;



	// 初期化
	void Initialize(WinApp* winApp);

	//描画前処理
	void PreDraw();

	//描画後処理
	void PostDraw();


	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);


	//--ゲッター--
	ID3D12Device* GetDevice() { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList_.Get(); }
	ID3D12CommandQueue* GetCommandQueue() { return commandQueue_.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() { return commandAllocator_.Get(); }
	IDXGISwapChain4* GetSwapChain() { return swapChain_.Get(); }
	ID3D12Resource* GetCurrentBackBuffer() {
		return swapChainResources_[swapChain_->GetCurrentBackBufferIndex()].Get();
	}
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() {
		return rtvHandles_[swapChain_->GetCurrentBackBufferIndex()];
	}
	ID3D12DescriptorHeap* GetSRVDescriptorHeap() { return srvDescriptorHeap_.Get(); }
	ID3D12DescriptorHeap* GetRTVDescriptorHeap() { return rtvDescriptorHeap_.Get(); }
	ID3D12DescriptorHeap* GetDSVDescriptorHeap() { return dsvDescriptorHeap_.Get(); }
	ID3D12Resource* GetDepthStencilBuffer() { return depthStencilResource_.Get(); }
	IDxcUtils* GetDxcUtils() { return dxcUtils_.Get(); }
	IDxcCompiler3* GetDxcCompiler() { return dxcCompiler_.Get(); }
	IDxcIncludeHandler* GetIncludeHandler() { return includeHandler_.Get(); }
	DXGI_FORMAT GetRTVFormat() { return rtvFormat_; }
	UINT64 GetFenceValue() { return fenceValue_; }
	HANDLE GetFenceEvent() { return fenceEvent_; }
	ID3D12Fence *GetFence() { return fence_.Get(); }
	D3D12_RECT GetScissorRect() { return scissorRect_; }
	D3D12_VIEWPORT GetViewport() { return viewport_; }
	IDxcIncludeHandler* GetDxcIncludeHandler() { return includeHandler_.Get(); }
	uint32_t GetSRVDescriptorSize() { return srvDescriptorSize_; }

	//セッター
	void IncrementFenceValue() { fenceValue_++; }


	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
private:
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

	// デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;


	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;

	// デスクリプタサイズ
	uint32_t rtvDescriptorSize_;
	uint32_t srvDescriptorSize_;
	uint32_t dsvDescriptorSize_;

	D3D12_RECT scissorRect_{};

	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;


	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources_;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> rtvHandles_;

	DXGI_FORMAT rtvFormat_;

	HANDLE fenceEvent_ = nullptr;
	UINT64 fenceValue_ = 0;

	D3D12_VIEWPORT viewport_{};



	WinApp* winApp_ = nullptr;

	//デバイスの初期化
	void CreateDevice();

	//コマンド関連の初期化
	void CreateCommand();

	//スワップチェーンの初期化
	void CreateSwapChain();

	//深度バッファの生成
	void CreateDepthStencilBuffer();

	//深度ステンシルビューの生成
	void CreateDepthStencilView();

	//各種デスクリプタヒープの生成
	void CreateDescriptorHeaps();

	//レンダーターゲットビューの初期化
	void CreateRenderTargetView();

	//フェンスの作成
	void CreateFence();

	//ビューポート矩形の初期化
	void SetViewport();

	//シザー矩形の初期化
	void SetScissorRect();

	//DXCコンパイラの初期化
	void InitializeDxcCompiler();

	//ImGuiの初期化
	void InitializeImGui();



	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);


};
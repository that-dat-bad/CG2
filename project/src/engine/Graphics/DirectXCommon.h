#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include"./engine/base/WinApp.h"
class DirectXCommon
{
public:

	static const uint32_t kRtvHeapDescriptorNum_ = 2; // ダブルバッファ用
	static const uint32_t kSrvHeapDescriptorNum_ = 100; // SRVの最大数
	static const uint32_t kDsvHeapDescriptorNum_ = 1; // 深度バッファ用




	// 初期化
	void Initialize(WinApp *winApp);

	//デバイスの初期化
	void CreateDevice();

	//コマンド関連の初期化
	void CreateCommand();

	//スワップチェーンの初期化
	void CreateSwapChain();

	//深度バッファの生成
	void CreateDepthStencilView();

	//各種デスクリプタヒープの生成
	void CreateDescriptorHeaps();

	//レンダーターゲットビューの初期化
	void CreateRenderTargetView();

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

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

	// デスクリプタサイズ
	uint32_t rtvDescriptorSize_;
	uint32_t srvDescriptorSize_;
	uint32_t dsvDescriptorSize_;

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

	WinApp* winApp_ = nullptr;
};


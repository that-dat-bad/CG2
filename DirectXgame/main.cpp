#include <Windows.h>
#include <cstdint>
#include <string>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <format>
#include <strsafe.h>
#include <dxcapi.h>
#include <vector>
#include "Matrix4x4.h"
#include"Vector3.h"
#include <math.h>
#define _USE_MATH_DEFINES
#include <fstream>
#include<sstream>
#include <map>
#include"DebugCamera.h"
#include "winApp.h"

// debug用のヘッダ
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib,"dxcompiler.lib")


#include "./externals/imgui/imgui.h"
#include "./externals/imgui/imgui_impl_dx12.h"
#include "./externals/imgui/imgui_impl_win32.h"

#include "./externals/DirectXTex/DirectXTex.h"
#include "./externals/DirectXTex/d3dx12.h"

#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#include <Xinput.h> // XInputのヘッダを追加
#pragma comment(lib, "xinput.lib") // XInputのライブラリをリンク


struct Vector4 { float x, y, z, w; };


struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};


struct Material {
	Vector4 color;
	int32_t enableLighting;
	float shininess;
	float padding[2];
	Matrix4x4 uvTransform; // UV変換行列
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};


struct LightingSettings {
	int32_t lightingModel; // 0: Lambert, 1: Half-Lambert
	float padding[3];
};

struct MaterialData {
	std::string textureFilePath;
	std::string name; // マテリアル名を追加
};

// 各メッシュの情報を保持する構造体
struct MeshObject {
	std::string name;
	std::vector<VertexData> vertices;
	MaterialData material; // このメッシュに適用されるマテリアル
	Transform transform; // このメッシュ固有のSRT
	Transform uvTransform; // メッシュ固有のUV変換
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
	TransformationMatrix* wvpData = nullptr;
	int textureAssetIndex = 0; // このメッシュが使用するテクスチャのインデックス
	bool hasUV = false; // このメッシュがUVを持つか
};

// 読み込んだモデルアセット (複数のメッシュを含む)
struct ModelData {
	std::string name;
	std::vector<MeshObject> meshes; // 複数のメッシュを保持
};

// 読み込んだテクスチャアセット
struct TextureAsset {
	std::string name;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	// テクスチャのメタデータを保持するためのフィールドを追加
	DirectX::TexMetadata metadata;
};

// 読み込んだモデルアセット (ModelDataを保持)
struct ModelAsset {
	ModelData modelData;
	// ModelAssetレベルでの頂点バッファは不要になる
};


// シーン内のオブジェクト (ModelAssetを参照し、その中のメッシュを管理)
struct GameObject {
	Transform transform; // オブジェクト全体の変換 (各メッシュに適用される)
	int modelAssetIndex = 0; // どのModelAssetを使用するか
};

struct ChunkHeader {
	char id[4];
	int32_t size;
};

struct RiffHeader {
	ChunkHeader chunk;
	char type[4];
};

struct FormatChunk {
	ChunkHeader chunk;
	WAVEFORMATEX fmt;
};

struct SoundData {
	WAVEFORMATEX wfex;
	BYTE* pBuffer;
	unsigned int buffersize;
};

#pragma region 関数群
//ログ用関数
void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

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

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* bufferResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferResource)
	);
	assert(SUCCEEDED(hr));
	return bufferResource;
}


Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height)
{
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));
	return resource;
}


static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dump", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dump/%04d-%02d-%02d-%02d-%02d.dmp",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	return EXCEPTION_EXECUTE_HANDLER;
}


Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile, IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler) {
	Log(ConvertString(std::format(L"Begin CompileShader, Path :{}, profile : {}\n", filePath, profile)));
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));
	DxcBuffer shaderSourceBuffer{};
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi",L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	assert(SUCCEEDED(hr));

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	Log(ConvertString(std::format(L"Compile Succeeded, Path :{}, profile : {}\n", filePath, profile)));
	return shaderBlob;
};

DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));
	return mipImages;
}

ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

[[nodiscard]]
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	ID3D12Resource* intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

// 複数のマテリアルを読み込むためのマップ
std::map<std::string, MaterialData> LoadMaterialTemplates(const std::string& directoryPath, const std::string& filename) {
	std::map<std::string, MaterialData> materials;
	std::string currentMaterialName;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		if (identifier == "newmtl") {
			s >> currentMaterialName;
			materials[currentMaterialName].name = currentMaterialName;
		}
		else if (identifier == "map_Kd" && !currentMaterialName.empty()) {
			std::string textureFileName;
			s >> textureFileName;
			materials[currentMaterialName].textureFilePath = directoryPath + "/" + textureFileName;
		}
	}
	return materials;
}

// OBJファイルを読み込み、複数のメッシュとして解析する関数
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename, ID3D12Device* device) {
	ModelData modeldata;
	modeldata.name = filename;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	std::map<std::string, MaterialData> loadedMaterials;
	MeshObject currentMesh;
	bool firstMesh = true;

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "o" || identifier == "g") { // 新しいオブジェクトまたはグループの開始
			if (!firstMesh) {
				// 以前のメッシュを保存
				if (!currentMesh.vertices.empty()) {
					// メッシュの頂点バッファを作成
					currentMesh.vertexBuffer = CreateBufferResource(device, sizeof(VertexData) * currentMesh.vertices.size());
					VertexData* mappedData = nullptr;
					currentMesh.vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
					std::memcpy(mappedData, currentMesh.vertices.data(), sizeof(VertexData) * currentMesh.vertices.size());
					currentMesh.vertexBuffer->Unmap(0, nullptr);

					currentMesh.vertexBufferView.BufferLocation = currentMesh.vertexBuffer->GetGPUVirtualAddress();
					currentMesh.vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * currentMesh.vertices.size());
					currentMesh.vertexBufferView.StrideInBytes = sizeof(VertexData);

					// マテリアルとWVPのリソースを作成
					currentMesh.materialResource = CreateBufferResource(device, sizeof(Material));
					currentMesh.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&currentMesh.materialData));
					currentMesh.materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルト色
					currentMesh.materialData->enableLighting = 1;
					currentMesh.materialData->shininess = 0.0f;
					currentMesh.materialData->uvTransform = Identity4x4(); // UV変換を単位行列に初期化

					currentMesh.wvpResource = CreateBufferResource(device, sizeof(TransformationMatrix));
					currentMesh.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&currentMesh.wvpData));
					currentMesh.transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // デフォルト変換
					currentMesh.uvTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // UV変換の初期化

					modeldata.meshes.push_back(currentMesh);
				}
			}
			firstMesh = false;
			currentMesh = MeshObject(); // 新しいメッシュを初期化
			s >> currentMesh.name; // メッシュ名を設定
			currentMesh.transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // デフォルト変換
			currentMesh.uvTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // UV変換の初期化
			currentMesh.hasUV = false; // 初期化
		}
		else if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			// 座標系の変換
			position.x *= -1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			// V方向の反転
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
			currentMesh.hasUV = true; // このメッシュはUVを持つ
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			// 座標系の変換
			normal.x *= -1.0f;
			normals.push_back(normal);
		}
		else if (identifier == "f") {
			VertexData faceVertices[3];
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;

				uint32_t elementIndices[3] = { 0, 0, 0 };

				size_t first_slash = vertexDefinition.find('/');
				elementIndices[0] = std::stoi(vertexDefinition.substr(0, first_slash));

				if (first_slash != std::string::npos) {
					if (vertexDefinition[first_slash + 1] != '/') {
						size_t second_slash = vertexDefinition.find('/', first_slash + 1);
						elementIndices[1] = std::stoi(vertexDefinition.substr(first_slash + 1, second_slash - (first_slash + 1)));
					}
					size_t second_slash = vertexDefinition.find('/', first_slash + 1);
					if (second_slash != std::string::npos) {
						elementIndices[2] = std::stoi(vertexDefinition.substr(second_slash + 1));
					}
				}

				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = { 0.0f, 0.0f };
				if (elementIndices[1] != 0 && currentMesh.hasUV) { // currentMesh.hasUV を使用
					texcoord = texcoords[elementIndices[1] - 1];
				}
				Vector3 normal = (elementIndices[2] > 0 && !normals.empty()) ? normals[elementIndices[2] - 1] : Vector3{ 0.0f, 0.0f, 1.0f };

				faceVertices[faceVertex] = { position, texcoord, normal };
			}
			currentMesh.vertices.push_back(faceVertices[0]);
			currentMesh.vertices.push_back(faceVertices[2]);
			currentMesh.vertices.push_back(faceVertices[1]);
		}
		else if (identifier == "mtllib") {
			std::string materialFileName;
			s >> materialFileName;
			loadedMaterials = LoadMaterialTemplates(directoryPath, materialFileName);
		}
		else if (identifier == "usemtl") {
			std::string materialName;
			s >> materialName;
			if (loadedMaterials.count(materialName)) {
				currentMesh.material = loadedMaterials[materialName];
			}
		}
	}
	// ファイルの終わりに残った最後のメッシュを保存
	if (!currentMesh.vertices.empty()) {
		currentMesh.vertexBuffer = CreateBufferResource(device, sizeof(VertexData) * currentMesh.vertices.size());
		VertexData* mappedData = nullptr;
		currentMesh.vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
		std::memcpy(mappedData, currentMesh.vertices.data(), sizeof(VertexData) * currentMesh.vertices.size());
		currentMesh.vertexBuffer->Unmap(0, nullptr);

		currentMesh.vertexBufferView.BufferLocation = currentMesh.vertexBuffer->GetGPUVirtualAddress();
		currentMesh.vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * currentMesh.vertices.size());
		currentMesh.vertexBufferView.StrideInBytes = sizeof(VertexData);

		currentMesh.materialResource = CreateBufferResource(device, sizeof(Material));
		currentMesh.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&currentMesh.materialData));
		currentMesh.materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルト色
		currentMesh.materialData->enableLighting = 1;
		currentMesh.materialData->shininess = 0.0f;
		currentMesh.materialData->uvTransform = Identity4x4(); // UV変換を単位行列に初期化

		currentMesh.wvpResource = CreateBufferResource(device, sizeof(TransformationMatrix));
		currentMesh.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&currentMesh.wvpData));
		currentMesh.transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // デフォルト変換
		currentMesh.uvTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // UV変換の初期化

		modeldata.meshes.push_back(currentMesh);
	}
	return modeldata;
}

SoundData SoundLoadWave(const char* filename) {
	std::ifstream file;
	file.open(filename, std::ios_base::binary);
	assert(file.is_open());
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) assert(0);
	if (strncmp(riff.type, "WAVE", 4) != 0) assert(0);
	FormatChunk format = {};
	file.read((char*)&format, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) assert(0);
	file.read((char*)&format.fmt, format.chunk.size);
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	if (strncmp(data.id, "JUNK", 4) == 0) {
		file.seekg(data.size, std::ios_base::cur);
		file.read((char*)&data, sizeof(data));
	}
	if (strncmp(data.id, "data", 4) != 0) assert(0);
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);
	file.close();
	SoundData soundData = {};
	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.buffersize = data.size;
	return soundData;
}

void SoundUnload(SoundData* soundData) {
	delete[] soundData->pBuffer;
	soundData->pBuffer = nullptr;
	soundData->buffersize = 0;
	soundData->wfex = {};
}

void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData) {
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	HRESULT result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));
	XAUDIO2_BUFFER buf = {};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.buffersize;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start(0);
}

#pragma endregion

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

	WinApp* winApp = new WinApp();
	// ウィンドウの初期化
	winApp->Initialize();
	SetUnhandledExceptionFilter(ExportDump);


#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

	// アダプタの選別
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			Log(ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	// D3D12Deviceの生成
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) {
			Log(std::format("FeatureLevel: {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device.As(&infoQueue))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		D3D12_MESSAGE_ID denyIds[] = { D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.pSeverityList = severities;
		infoQueue->PushStorageFilter(&filter);
	}
#endif

	// コマンドキュー、コマンドアロケータ、コマンドリストの生成
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(hr));

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));

	// スワップチェーンの生成
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = winApp->kClientWidth;
	swapChainDesc.Height = winApp->kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> tempSwapChain;
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc, nullptr, nullptr, &tempSwapChain);
	assert(SUCCEEDED(hr));
	hr = tempSwapChain.As(&swapChain);
	assert(SUCCEEDED(hr));


	// RTVディスクリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2];
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	for (UINT i = 0; i < 2; i++) {
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
		assert(SUCCEEDED(hr));
		rtvHandles[i] = GetCPUDescriptorHandle(rtvDescriptorHeap.Get(), device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), i);
		device->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc, rtvHandles[i]);
	}

	// DSVディスクリプタヒープとリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = CreateDepthStencilTextureResource(device.Get(), winApp->kClientWidth, winApp->kClientHeight);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// フェンスの生成
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

	// dxcCompilerの初期化
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	// RootSignature
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[5] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0; // for Material
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 1; // for WVP
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1; // for DirectionalLight
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[4].Descriptor.ShaderRegister = 2; // for LightingSettings

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	// PSO
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[2] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicsPipelineStateDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	graphicsPipelineStateDesc.BlendState = blendDesc;

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	// SRVディスクリプタヒープ
	const uint32_t kNumMaxTextures = 128;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kNumMaxTextures, true);
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// テクスチャ読み込み
	std::vector<TextureAsset> textureAssets;
	std::vector<std::string> texturePaths = {
		"resources/uvchecker.png",
		"resources/monsterBall.png",
		"resources/checkerBoard.png",
	};

	// ImGui用に1つ予約
	uint32_t srvIndex = 1;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateResources;

	for (const auto& path : texturePaths) {
		DirectX::ScratchImage mipImages = LoadTexture(path);
		const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(device.Get(), metadata);
		intermediateResources.push_back(UploadTextureData(textureResource.Get(), mipImages, device.Get(), commandList.Get()));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		// ここを修正: テクスチャがsRGBの場合、SRVのフォーマットを_SRGBにする
		if (metadata.format == DXGI_FORMAT_R8G8B8A8_UNORM) { // 一般的なPNGのフォーマット
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		else {
			srvDesc.Format = metadata.format;
		}
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

		TextureAsset newAsset;
		newAsset.name = path;
		newAsset.resource = textureResource;
		newAsset.cpuHandle = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, srvIndex);
		newAsset.gpuHandle = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, srvIndex);
		newAsset.metadata = metadata; // メタデータを保存
		device->CreateShaderResourceView(newAsset.resource.Get(), &srvDesc, newAsset.cpuHandle);
		textureAssets.push_back(newAsset);
		srvIndex++;
	}

	// モデル読み込み
	std::vector<ModelAsset> modelAssets;
	std::vector<std::string> modelPaths = {
		"sphere.obj",
		"plane.obj",
		//"teapot.obj",
		//"bunny.obj",
		//"suzanne.obj",
		//"multiMesh.obj",
		//"multiMaterial.obj"
	};
	for (const auto& filename : modelPaths) {
		ModelData modelData = LoadObjFile("resources", filename, device.Get());
		ModelAsset newAsset;
		newAsset.modelData = modelData;
		modelAssets.push_back(newAsset);
	}

	// ゲームオブジェクトの初期化 (複数オブジェクトに対応)
	std::vector<GameObject> gameObjects;
	GameObject obj1;
	obj1.transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // 初期位置を中央に
	obj1.modelAssetIndex = 0; // Sphere
	gameObjects.push_back(obj1);

	GameObject obj2;
	obj2.transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f} }; // 2つ目のオブジェクトをX軸にオフセット
	obj2.modelAssetIndex = 2; // Teapot
	gameObjects.push_back(obj2);

	// ImGuiで選択されているメッシュのインデックス
	int selectedMeshIndex = 0;

	// スプライトの初期化
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Transform uvTransformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }; // スプライトのUV変換
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = CreateBufferResource(device.Get(), sizeof(VertexData) * 6);
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = CreateBufferResource(device.Get(), sizeof(uint32_t) * 6);
	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
	indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;
	indexResourceSprite->Unmap(0, nullptr);
	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	// スプライトの頂点座標をテクスチャの原寸サイズに設定
	// 初期テクスチャのサイズを取得
	float initialSpriteWidth = static_cast<float>(textureAssets[0].metadata.width);
	float initialSpriteHeight = static_cast<float>(textureAssets[0].metadata.height);

	vertexDataSprite[0].position = { 0.0f, initialSpriteHeight, 0.0f, 1.0f };   vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };      vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[2].position = { initialSpriteWidth, initialSpriteHeight, 0.0f, 1.0f };  vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
	vertexDataSprite[3].position = { initialSpriteWidth, 0.0f, 0.0f, 1.0f };    vertexDataSprite[3].texcoord = { 1.0f, 0.0f };
	vertexResourceSprite->Unmap(0, nullptr);
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4; // 4 vertices for 2 triangles
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = CreateBufferResource(device.Get(), sizeof(Material));
	Material* materialDataSprite = nullptr;
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	// スプライトのマテリアルデータを初期化
	materialDataSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色に設定
	materialDataSprite->enableLighting = 0; // ライティングを無効化
	materialDataSprite->shininess = 0.0f; // テクスチャサンプリングのために0.0f以上を設定
	materialDataSprite->uvTransform = Identity4x4(); // UV変換を単位行列に設定

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourceSprite = CreateBufferResource(device.Get(), sizeof(TransformationMatrix));
	TransformationMatrix* wvpDataSprite = nullptr;
	wvpResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataSprite));
	int spriteTextureIndex = 0;
	bool isSpriteVisible = false;

	// ライトの初期化
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = CreateBufferResource(device.Get(), sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, 1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;

	// ライティング設定の初期化
	Microsoft::WRL::ComPtr<ID3D12Resource> lightingSettingsResource = CreateBufferResource(device.Get(), sizeof(LightingSettings));
	LightingSettings* lightingSettingsData = nullptr;
	lightingSettingsResource->Map(0, nullptr, reinterpret_cast<void**>(&lightingSettingsData));
	lightingSettingsData->lightingModel = 0; // Lambert by default (0: Lambert, 1: Half-Lambert)

	// コマンドを確定させて待つ
	hr = commandList->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandLists);
	fenceValue++;
	commandQueue->Signal(fence.Get(), fenceValue);
	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));

	// ビューポートとシザー矩形
	D3D12_VIEWPORT viewport{};
	viewport.Width = winApp->kClientWidth; viewport.Height = winApp->kClientHeight;
	viewport.TopLeftX = 0; viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f; viewport.MaxDepth = 1.0f;
	D3D12_RECT scissorRect{};
	scissorRect.left = 0; scissorRect.right = winApp->kClientWidth;
	scissorRect.top = 0; scissorRect.bottom = winApp->kClientHeight;

	// ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());
	ImGui_ImplDX12_Init(device.Get(), swapChainDesc.BufferCount,
		rtvDesc.Format, srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// 入力とカメラ
	Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;
	DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);

	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard = nullptr;
	directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	keyboard->SetDataFormat(&c_dfDIKeyboard);
	keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);

	Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse = nullptr;
	directInput->CreateDevice(GUID_SysMouse, &mouse, NULL);
	mouse->SetDataFormat(&c_dfDIMouse2);
	mouse->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	DebugCamera g_debugCamera;

	// XAudio2の初期化
	IXAudio2* xAudio2 = nullptr;
	HRESULT hrXAudio2 = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hrXAudio2));

	IXAudio2MasteringVoice* masteringVoice = nullptr;
	hrXAudio2 = xAudio2->CreateMasteringVoice(&masteringVoice);
	assert(SUCCEEDED(hrXAudio2));

	// サウンドデータの読み込み
	SoundData alarmSound = SoundLoadWave("resources/Alarm01.wav");


	// ImGuiのコンボボックスの状態を保持する変数
	int selectedLightingOption = 0; // 0: Lambert, 1: Half-Lambert, 2: None

	while (true) {

		if (winApp->ProcessMessage()) {
			break;
		}

		// ImGuiフレーム開始
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 入力取得
		keyboard->Acquire();
		mouse->Acquire();
		BYTE keys[256] = {};
		keyboard->GetDeviceState(sizeof(keys), keys);
		DIMOUSESTATE2 mouseState = {};
		mouse->GetDeviceState(sizeof(mouseState), &mouseState);

		// Gamepad input
		XINPUT_STATE gamepadState;
		ZeroMemory(&gamepadState, sizeof(XINPUT_STATE));
		DWORD dwResult = XInputGetState(0, &gamepadState); // Get the state of player 1's gamepad

		// カメラ更新
		g_debugCamera.Update(keys, mouseState);

		// Gamepadで最初のモデルの回転を操作
		if (dwResult == ERROR_SUCCESS && !gameObjects.empty()) // Gamepad is connected and at least one object exists
		{
			float rotationSpeed = 0.05f; // 回転速度

			// 右スティックのX軸でY軸回転
			if (abs(gamepadState.Gamepad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
				gameObjects[0].transform.rotate.y += static_cast<float>(gamepadState.Gamepad.sThumbRX) / SHRT_MAX * rotationSpeed;
			}
			// 右スティックのY軸でX軸回転
			if (abs(gamepadState.Gamepad.sThumbRY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
				gameObjects[0].transform.rotate.x += static_cast<float>(gamepadState.Gamepad.sThumbRY) / SHRT_MAX * rotationSpeed;
			}
		}

		// ImGuiウィンドウ
		ImGui::Begin("Settings");
		{
			// グローバル設定
			ImGui::SeparatorText("Global Settings");
			// ライティングモデルの選択肢に「None」を追加
			const char* lightingItems[] = { "Lambert", "Half-Lambert", "None" };
			ImGui::Combo("Lighting Model", &selectedLightingOption, lightingItems, IM_ARRAYSIZE(lightingItems));

			// selectedLightingOptionに基づいてenableLightingとlightingModelを設定
			if (selectedLightingOption == 2) { // Noneが選択された場合
				// 全てのメッシュのライティングを無効にする
				for (auto& gameObject : gameObjects) {
					if (gameObject.modelAssetIndex >= 0 && gameObject.modelAssetIndex < modelAssets.size()) {
						for (auto& mesh : modelAssets[gameObject.modelAssetIndex].modelData.meshes) {
							mesh.materialData->enableLighting = 0;
						}
					}
				}
			}
			else { // LambertまたはHalf-Lambertが選択された場合
				// 全てのメッシュのライティングを有効にする
				for (auto& gameObject : gameObjects) {
					if (gameObject.modelAssetIndex >= 0 && gameObject.modelAssetIndex < modelAssets.size()) {
						for (auto& mesh : modelAssets[gameObject.modelAssetIndex].modelData.meshes) {
							mesh.materialData->enableLighting = 1;
						}
					}
				}
				lightingSettingsData->lightingModel = selectedLightingOption; // 選択されたモデルを設定
			}

			// ライトの色を調整できるようにする
			ImGui::ColorEdit4("Light Color", &directionalLightData->color.x);

			// enableLightingが有効な場合のみ、ライトの方向を調整可能にする
			// ここでは、最初のオブジェクトのライティング設定を代表として使用
			if (!gameObjects.empty() && gameObjects[0].modelAssetIndex >= 0 && gameObjects[0].modelAssetIndex < modelAssets.size() &&
				!modelAssets[gameObjects[0].modelAssetIndex].modelData.meshes.empty() &&
				modelAssets[gameObjects[0].modelAssetIndex].modelData.meshes[0].materialData->enableLighting != 0) {
				ImGui::SliderFloat3("Light Direction", &directionalLightData->direction.x, -1.0f, 1.0f);
				directionalLightData->direction = Normalize(directionalLightData->direction);
			}
			else {
				ImGui::Text("Light Direction: N/A (Lighting Disabled)");
			}

			// オーディオ設定
			ImGui::SeparatorText("Audio Settings");
			if (ImGui::Button("Play Alarm Sound")) {
				SoundPlayWave(xAudio2, alarmSound);
			}

			// スプライト設定
			ImGui::SeparatorText("Sprite Settings");
			ImGui::Checkbox("Show Sprite", &isSpriteVisible);
			if (isSpriteVisible) {
				std::vector<const char*> textureNames;
				for (const auto& asset : textureAssets) { textureNames.push_back(asset.name.c_str()); }
				ImGui::Combo("Sprite Texture", &spriteTextureIndex, textureNames.data(), static_cast<int>(textureNames.size()));
				ImGui::DragFloat3("Sprite Pos", &transformSprite.translate.x, 1.0f);

				// スプライトのUV変換
				ImGui::DragFloat3("Sprite UV Scale", &uvTransformSprite.scale.x, 0.01f, 0.01f, 10.0f);
				ImGui::SliderAngle("Sprite UV Rotate Z", &uvTransformSprite.rotate.z);
				ImGui::DragFloat3("Sprite UV Translate", &uvTransformSprite.translate.x, 0.01f);

				// スプライトのサイズをテクスチャの原寸に合わせる
				float currentSpriteWidth = static_cast<float>(textureAssets[spriteTextureIndex].metadata.width);
				float currentSpriteHeight = static_cast<float>(textureAssets[spriteTextureIndex].metadata.height);

				// 頂点バッファを再マップして更新
				vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
				vertexDataSprite[0].position = { 0.0f, currentSpriteHeight, 0.0f, 1.0f };   vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
				vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };      vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
				vertexDataSprite[2].position = { currentSpriteWidth, currentSpriteHeight, 0.0f, 1.0f };  vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
				vertexDataSprite[3].position = { currentSpriteWidth, 0.0f, 0.0f, 1.0f };    vertexDataSprite[3].texcoord = { 1.0f, 0.0f };
				vertexResourceSprite->Unmap(0, nullptr);
			}

			// オブジェクト設定 (複数オブジェクト)
			ImGui::SeparatorText("Object Settings");
			for (int i = 0; i < gameObjects.size(); ++i) {
				GameObject& currentGameObject = gameObjects[i];
				ImGui::PushID(i); // 各オブジェクトのコントロールにユニークなIDをプッシュ
				ImGui::SeparatorText(std::format("Object {}", i + 1).c_str());

				std::vector<const char*> modelNames;
				for (const auto& asset : modelAssets) { modelNames.push_back(asset.modelData.name.c_str()); }
				ImGui::Combo("Model", &currentGameObject.modelAssetIndex, modelNames.data(), static_cast<int>(modelNames.size()));

				// 選択中のモデルのマテリアルカラーを調整する
				if (currentGameObject.modelAssetIndex >= 0 && currentGameObject.modelAssetIndex < modelAssets.size() &&
					!modelAssets[currentGameObject.modelAssetIndex].modelData.meshes.empty() &&
					modelAssets[currentGameObject.modelAssetIndex].modelData.meshes[0].materialData) {
					// 最初のメッシュのマテリアルカラーをImGuiで編集可能にする
					Material* currentMaterial = modelAssets[currentGameObject.modelAssetIndex].modelData.meshes[0].materialData;
					ImGui::ColorEdit4("Material Color", &currentMaterial->color.x);

					// Debug output for material color
					Log(std::format("Current Model: {}, Material Color: R:{:.2f}, G:{:.2f}, B:{:.2f}, A:{:.2f}\n",
						modelAssets[currentGameObject.modelAssetIndex].modelData.name,
						currentMaterial->color.x, currentMaterial->color.y, currentMaterial->color.z, currentMaterial->color.w));
				}

				// オブジェクト全体の変換
				ImGui::DragFloat3("Position", &currentGameObject.transform.translate.x, 0.1f);
				ImGui::DragFloat3("Scale", &currentGameObject.transform.scale.x, 0.1f);
				ImGui::SliderAngle("Rotate X", &currentGameObject.transform.rotate.x);
				ImGui::SliderAngle("Rotate Y", &currentGameObject.transform.rotate.y);
				ImGui::SliderAngle("Rotate Z", &currentGameObject.transform.rotate.z);
				ImGui::PopID(); // ユニークなIDをポップ
			}
		}
		ImGui::End(); // Global Settings End

		// multimesh.objが選択されている場合のみMesh Settingsを表示
		// このセクションは、最初のゲームオブジェクトがmultiMesh.objの場合にのみ表示されます。
		if (!gameObjects.empty() && gameObjects[0].modelAssetIndex >= 0 && gameObjects[0].modelAssetIndex < modelAssets.size() &&
			modelAssets[gameObjects[0].modelAssetIndex].modelData.name == "multiMesh.obj")
		{
			ImGui::Begin("Mesh Settings (Object 1)"); // 特定のオブジェクトに紐づける
			{
				// メッシュごとの設定
				ModelData& currentModel = modelAssets[gameObjects[0].modelAssetIndex].modelData; // 最初のオブジェクトのモデル
				if (!currentModel.meshes.empty()) {
					std::vector<const char*> meshNames;
					for (const auto& mesh : currentModel.meshes) { meshNames.push_back(mesh.name.c_str()); }
					// selectedMeshIndexが現在のメッシュ数を超えないように調整
					if (selectedMeshIndex >= currentModel.meshes.size()) {
						selectedMeshIndex = 0; // 無効な場合は0番目のメッシュを選択
					}
					ImGui::Combo("Select Mesh", &selectedMeshIndex, meshNames.data(), static_cast<int>(meshNames.size()));

					if (selectedMeshIndex >= 0 && selectedMeshIndex < currentModel.meshes.size()) {
						MeshObject& selectedMesh = currentModel.meshes[selectedMeshIndex];

						ImGui::SeparatorText(selectedMesh.name.c_str());
						ImGui::DragFloat3("Mesh Position", &selectedMesh.transform.translate.x, 0.1f);
						ImGui::DragFloat3("Mesh Scale", &selectedMesh.transform.scale.x, 0.1f);
						ImGui::SliderAngle("Mesh Rotate X", &selectedMesh.transform.rotate.x);
						ImGui::SliderAngle("Mesh Rotate Y", &selectedMesh.transform.rotate.y);
						ImGui::SliderAngle("Mesh Rotate Z", &selectedMesh.transform.rotate.z);
						ImGui::ColorEdit4("Mesh Color", &selectedMesh.materialData->color.x);

						// メッシュがUVを持っている場合のみテクスチャ選択とUV変換を有効にする
						if (selectedMesh.hasUV) {
							std::vector<const char*> textureNames;
							for (const auto& asset : textureAssets) { textureNames.push_back(asset.name.c_str()); }
							// int から size_t に変更
							size_t meshTexIdx = selectedMesh.textureAssetIndex;
							ImGui::Combo("Mesh Texture", reinterpret_cast<int*>(&meshTexIdx), textureNames.data(), static_cast<int>(textureNames.size()));
							selectedMesh.textureAssetIndex = static_cast<int>(meshTexIdx); // intに戻す

							// UV変換のImGuiコントロール
							ImGui::SeparatorText("Mesh UV Transform");
							ImGui::DragFloat3("Mesh UV Scale", &selectedMesh.uvTransform.scale.x, 0.01f, 0.01f, 10.0f);
							ImGui::SliderAngle("Mesh UV Rotate Z", &selectedMesh.uvTransform.rotate.z);
							ImGui::DragFloat3("Mesh UV Translate", &selectedMesh.uvTransform.translate.x, 0.01f);

						}
						else {
							ImGui::Text("Mesh Texture: N/A (No UVs)");
							ImGui::Text("Mesh UV Transform: N/A (No UVs)");
						}
					}
				}
				else {
					ImGui::Text("No meshes in this model.");
				}
			}
			ImGui::End(); // Mesh Settings End
		}


		// 更新処理
		const Matrix4x4& viewMatrix = g_debugCamera.GetViewMatrix();
		Matrix4x4 projectionMatrix = MakePerspectiveMatrix(0.45f, float(winApp->kClientWidth) / float(winApp->kClientHeight), 0.1f, 100.0f);

		// 各ゲームオブジェクトの更新
		for (auto& gameObject : gameObjects) {
			// オブジェクト全体のワールド行列
			Matrix4x4 globalWorldMatrix = MakeAffineMatrix(gameObject.transform.scale, gameObject.transform.rotate, gameObject.transform.translate);

			// 各メッシュの更新
			if (gameObject.modelAssetIndex >= 0 && gameObject.modelAssetIndex < modelAssets.size()) {
				ModelData& currentModel = modelAssets[gameObject.modelAssetIndex].modelData;
				for (auto& mesh : currentModel.meshes) {
					// メッシュ固有のワールド行列
					Matrix4x4 meshLocalWorldMatrix = MakeAffineMatrix(mesh.transform.scale, mesh.transform.rotate, mesh.transform.translate);
					// グローバルなオブジェクト変換とメッシュ固有の変換を結合
					Matrix4x4 finalWorldMatrix = Multiply(meshLocalWorldMatrix, globalWorldMatrix);

					mesh.wvpData->World = finalWorldMatrix;
					mesh.wvpData->WVP = Multiply(finalWorldMatrix, Multiply(viewMatrix, projectionMatrix));

					// メッシュのUV変換行列を更新
					if (mesh.hasUV) {
						mesh.materialData->uvTransform = MakeAffineMatrix(mesh.uvTransform.scale, mesh.uvTransform.rotate, mesh.uvTransform.translate);
					}
					else {
						mesh.materialData->uvTransform = Identity4x4(); // UVがない場合は単位行列
					}
				}
			}
		}


		if (isSpriteVisible) {
			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = Identity4x4();
			Matrix4x4 projectionMatrixSprite = makeOrthographicmMatrix(0.0f, 0.0f, float(winApp->kClientWidth), float(winApp->kClientHeight), 0.0f, 100.0f);
			wvpDataSprite->World = worldMatrixSprite;
			wvpDataSprite->WVP = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));

			// スプライトのマテリアルデータを更新（毎フレーム確実に設定）
			materialDataSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色に設定
			materialDataSprite->enableLighting = 0; // ライティングを無効化
			materialDataSprite->shininess = 0.0f; // テクスチャサンプリングのために0.0f以上を設定
			// スプライトのUV変換行列を更新
			materialDataSprite->uvTransform = MakeAffineMatrix(uvTransformSprite.scale, uvTransformSprite.rotate, uvTransformSprite.translate);
		}

		// 描画処理
		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->SetPipelineState(graphicsPipelineState.Get());
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);
		commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(4, lightingSettingsResource->GetGPUVirtualAddress());

		// 3Dオブジェクト描画 (各ゲームオブジェクトの各メッシュをループして描画)
		for (auto& gameObject : gameObjects) {
			if (gameObject.modelAssetIndex >= 0 && gameObject.modelAssetIndex < modelAssets.size()) {
				ModelData& currentModel = modelAssets[gameObject.modelAssetIndex].modelData;
				for (auto& mesh : currentModel.meshes) {
					commandList->SetGraphicsRootConstantBufferView(0, mesh.materialResource->GetGPUVirtualAddress());
					commandList->SetGraphicsRootConstantBufferView(1, mesh.wvpResource->GetGPUVirtualAddress());
					commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);

					// メッシュがUVを持つか、またはマテリアルにテクスチャパスがあるかを確認
					if (mesh.hasUV && !mesh.material.textureFilePath.empty()) {
						// 適切なテクスチャアセットを見つけるロジック
						size_t meshTexIdx = 0;
						for (size_t i = 0; i < textureAssets.size(); ++i) {
							// マテリアルのテクスチャファイルパスとテクスチャアセットの名前を比較
							if (textureAssets[i].name == mesh.material.textureFilePath) {
								meshTexIdx = i;
								break;
							}
						}
						commandList->SetGraphicsRootDescriptorTable(2, textureAssets[meshTexIdx].gpuHandle);
					}
					else {
						// UVがないかテクスチャが指定されていない場合はデフォルトのテクスチャ (uvchecker.png) を使用
						commandList->SetGraphicsRootDescriptorTable(2, textureAssets[0].gpuHandle);
					}
					commandList->DrawInstanced(UINT(mesh.vertices.size()), 1, 0, 0);
				}
			}
		}


		// スプライト描画
		if (isSpriteVisible) {
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, wvpResourceSprite->GetGPUVirtualAddress());
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
			commandList->IASetIndexBuffer(&indexBufferViewSprite);
			commandList->SetGraphicsRootDescriptorTable(2, textureAssets[spriteTextureIndex].gpuHandle);
			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
		}

		// ImGui描画
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList->ResourceBarrier(1, &barrier);

		hr = commandList->Close();
		assert(SUCCEEDED(hr));
		ID3D12CommandList* commandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);
		swapChain->Present(1, 0);

		fenceValue++;
		commandQueue->Signal(fence.Get(), fenceValue);
		if (fence->GetCompletedValue() < fenceValue) {
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		hr = commandAllocator->Reset();
		assert(SUCCEEDED(hr));
		hr = commandList->Reset(commandAllocator.Get(), nullptr);
		assert(SUCCEEDED(hr));
	}


	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	SoundUnload(&alarmSound); // サウンドデータの解放
	if (masteringVoice) {
		masteringVoice->DestroyVoice();
	}
	if (xAudio2) {
		xAudio2->Release();
	}

	CloseHandle(fenceEvent);
	CoUninitialize();
	return 0;
}

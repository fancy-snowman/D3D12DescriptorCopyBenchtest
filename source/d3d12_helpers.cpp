#include "d3d12_helpers.hpp"

bool InitializeD3D12(ComPtr<ID3D12Device>& device, ComPtr<IDXGIFactory4>& factory) {
	HRESULT hr;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (FAILED(hr)) {
		std::cerr << "Failed to create DXGI Factory." << std::endl;
		return false;
	}

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex) {
		DXGI_ADAPTER_DESC1 desc;
		hardwareAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
			break;
		}
	}

	if (!device) {
		std::cerr << "Failed to create D3D12 Device." << std::endl;
		return false;
	}

	return true;
}

bool CreateTextures(ComPtr<ID3D12Device> device, UINT numTextures, ComPtr<ID3D12Heap>& heapOut, std::vector<ComPtr<ID3D12Resource>>& texturesOut)
{
	const UINT TEXTURE_WIDTH = 128;
	const UINT TEXTURE_HEIGHT = 128;
	HRESULT hr = S_OK;

	D3D12_RESOURCE_DESC textureDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Alignment = 0,
		.Width = TEXTURE_WIDTH,
		.Height = TEXTURE_HEIGHT,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};

	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = device->GetResourceAllocationInfo(0, 1, &textureDesc);

	D3D12_HEAP_DESC heapDesc = {
		.SizeInBytes = allocationInfo.SizeInBytes * numTextures,
		.Properties = { .Type = D3D12_HEAP_TYPE_DEFAULT },
		.Alignment = 0,
		.Flags = D3D12_HEAP_FLAG_NONE
	};

	ComPtr<ID3D12Heap> heap;
	hr = device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap));
	if (FAILED(hr))
	{
		return false;
	}

	std::vector<ComPtr<ID3D12Resource>> textures;
	textures.reserve(numTextures);

	for (UINT i = 0; i < numTextures; i++) {
		ComPtr<ID3D12Resource> texture;
		hr = device->CreatePlacedResource(heap.Get(),
			i * allocationInfo.SizeInBytes,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&texture));

		if (FAILED(hr))
		{
			return false;
		}
		textures.push_back(texture);
	}

	heapOut = std::move(heap);
	texturesOut = std::move(textures);
	return true;
}

bool CreateDescriptorHeap(ComPtr<ID3D12Device> device, HeapVisibility visibility, UINT numDescriptors, ComPtr<ID3D12DescriptorHeap>& heapOut)
{
	HRESULT hr = S_OK;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = numDescriptors,
		.Flags = visibility == HeapVisibility::Shader ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	ComPtr<ID3D12DescriptorHeap> heap;
	hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
	if (FAILED(hr))
	{
		return false;
	}

	heapOut = std::move(heap);
	return true;
}

void CreateDescriptors(ComPtr<ID3D12Device> device, std::span<ComPtr<ID3D12Resource>> resources, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleStart)
{
	const UINT DESCRIPTOR_STRIDE = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	HRESULT hr = S_OK;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Texture2D = {
			.MostDetailedMip = 0,
			.MipLevels = 1,
			.PlaneSlice = 0,
			.ResourceMinLODClamp = 0.0f
		}
	};

	for (size_t i = 0; i < resources.size(); i++)
	{
		const ComPtr<ID3D12Resource>& resource = resources[i];
		const D3D12_CPU_DESCRIPTOR_HANDLE descriptor = {
			.ptr = descriptorHandleStart.ptr + i * DESCRIPTOR_STRIDE
		};

		device->CreateShaderResourceView(resource.Get(), &srvDesc, descriptor);
	}
}

// Tests for helpers start here

TEST(D3D12Setup, Initialization) {
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory4> factory;

	ASSERT_TRUE(InitializeD3D12(device, factory));
	ASSERT_NE(device, nullptr);
	ASSERT_NE(factory, nullptr);
}

TEST(D3D12Setup, TextureCreation) {
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory4> factory;
	ASSERT_TRUE(InitializeD3D12(device, factory));

	const UINT NUM_TEXTURES = 10;

	ComPtr<ID3D12Heap> heap;
	std::vector<ComPtr<ID3D12Resource>> textures;
	ASSERT_TRUE(CreateTextures(device, NUM_TEXTURES, heap, textures));

	ASSERT_NE(heap, nullptr);
	ASSERT_EQ(textures.size(), NUM_TEXTURES);

	bool allTexturesValid = true;
	for (const auto& t : textures)
	{
		allTexturesValid &= (t != nullptr);
	}
	ASSERT_TRUE(allTexturesValid);
}

TEST(D3D12Setup, DescriptorHeapCreation) {
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory4> factory;
	ASSERT_TRUE(InitializeD3D12(device, factory));

	ComPtr<ID3D12DescriptorHeap> heap;
	ASSERT_TRUE(CreateDescriptorHeap(device, HeapVisibility::Shader, 10, heap));
	ASSERT_NE(heap, nullptr);

	heap.Reset();
	ASSERT_TRUE(CreateDescriptorHeap(device, HeapVisibility::NonShader, 10, heap));
	ASSERT_NE(heap, nullptr);
}

TEST(D3D12Setup, DescriptorCreation) {
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory4> factory;
	ASSERT_TRUE(InitializeD3D12(device, factory));

	UINT NUM_TEXTURES = 10;

	ComPtr<ID3D12Heap> heap;
	std::vector<ComPtr<ID3D12Resource>> textures;
	ASSERT_TRUE(CreateTextures(device, NUM_TEXTURES, heap, textures));

	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	ASSERT_TRUE(CreateDescriptorHeap(device, HeapVisibility::Shader, NUM_TEXTURES, descriptorHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	CreateDescriptors(device, textures, descriptorHandleStart);
}

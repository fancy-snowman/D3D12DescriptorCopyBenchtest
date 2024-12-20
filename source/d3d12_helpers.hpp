#include <gtest/gtest.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

using namespace Microsoft::WRL;

enum class HeapVisibility
{
	Shader,
	NonShader
};

bool InitializeD3D12(ComPtr<ID3D12Device>& device, ComPtr<IDXGIFactory4>& factory);
bool CreateTextures(ComPtr<ID3D12Device> device, UINT numTextures, ComPtr<ID3D12Heap>& heapOut, std::vector<ComPtr<ID3D12Resource>>& texturesOut);
bool CreateDescriptorHeap(ComPtr<ID3D12Device> device, HeapVisibility visibility, UINT numDescriptors, ComPtr<ID3D12DescriptorHeap>& heapOut);
void CreateDescriptors(ComPtr<ID3D12Device> device, std::span<ComPtr<ID3D12Resource>> resources, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleStart);

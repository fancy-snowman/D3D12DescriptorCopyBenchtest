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

struct CommandInterface
{
	// Objects to record and execute commands
	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12GraphicsCommandList> CommandList;
	ComPtr<ID3D12CommandAllocator> CommandAllocator;

	// Objects to synchronize CPU and GPU
	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent = nullptr;
	UINT64 FenceValue = 0;

	// Objects to measure GPU execution time
	ComPtr<ID3D12QueryHeap> QueryHeap;
	ComPtr<ID3D12Resource> QueryResult;

	void Reset()
	{
		CommandAllocator->Reset();
		CommandList->Reset(CommandAllocator.Get(), nullptr);
		CommandList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
	}

	void Execute()
	{
		CommandList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
		CommandList->Close();
		ID3D12CommandList* commandLists[] = { CommandList.Get() };
		CommandQueue->ExecuteCommandLists(1, commandLists);
	}

	void WaitForCompletion()
	{
		FenceValue++;
		CommandQueue->Signal(Fence.Get(), FenceValue);
		Fence->SetEventOnCompletion(FenceValue, FenceEvent);
		WaitForSingleObject(FenceEvent, INFINITE);
	}

	float RetrieveExecutionTime()
	{
		UINT64 timestamps[2];
		D3D12_RANGE readRange = { 0, sizeof(timestamps) };
		QueryResult->Map(0, &readRange, reinterpret_cast<void**>(&timestamps));
		QueryResult->Unmap(0, nullptr);
		return timestamps[1] - timestamps[0];
	}
};

bool InitializeD3D12(ComPtr<ID3D12Device>& device, ComPtr<IDXGIFactory4>& factory);
bool CreateTextures(ComPtr<ID3D12Device> device, UINT numTextures, ComPtr<ID3D12Heap>& heapOut, std::vector<ComPtr<ID3D12Resource>>& texturesOut);
bool CreateDescriptorHeap(ComPtr<ID3D12Device> device, HeapVisibility visibility, UINT numDescriptors, ComPtr<ID3D12DescriptorHeap>& heapOut);
void CreateDescriptors(ComPtr<ID3D12Device> device, std::span<ComPtr<ID3D12Resource>> resources, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleStart);
bool CreateCommandInterface(ComPtr<ID3D12Device> device, CommandInterface& commandInterfaceOut);

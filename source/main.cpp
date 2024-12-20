#include <gtest/gtest.h>
#include "d3d12_helpers.hpp"

#include <chrono>
#include <iostream>

struct TestTimer
{
	using Clock = std::chrono::high_resolution_clock;

	Clock::time_point StartPoint;
	Clock::time_point EndPoint;

	void Reset()
	{
		EndPoint = Clock::now();
		StartPoint = Clock::now();
	}

	void Stop()
	{
		EndPoint = Clock::now();
	}

	float GetElapsedMilliseconds()
	{
		return std::chrono::duration<float, std::milli>(EndPoint - StartPoint).count();
	}
};

struct TestContext
{
	ComPtr<ID3D12Device> Device;
	ComPtr<IDXGIFactory4> Factory;

	ComPtr<ID3D12Heap> TextureHeap;
	std::vector<ComPtr<ID3D12Resource>> Textures;
};

bool SetupTest(UINT numTextures, TestContext& setup)
{
	if (!InitializeD3D12(setup.Device, setup.Factory))
	{
		return false;
	}

	if (!CreateTextures(setup.Device, numTextures, setup.TextureHeap, setup.Textures))
	{
		return false;
	}

	return true;
}

struct TestDescription
{
	std::string Name;
	
	// Returns true if test was successful
	std::function<bool(TestContext&, D3D12_CPU_DESCRIPTOR_HANDLE /*descriptorDestination*/, float& /*executionTimeOut*/)> Run;
};

TEST(D3D12Test, RunTests) {

	std::vector<TestDescription> tests;

	{
		TestDescription desc;
		desc.Name = "CreateInDestinationHeap";
		desc.Run = [](TestContext& context, D3D12_CPU_DESCRIPTOR_HANDLE descriptorDestination, float& executionTimeOut) {
			TestTimer timer;
			timer.Reset();

			CreateDescriptors(context.Device, context.Textures, descriptorDestination);

			timer.Stop();
			executionTimeOut = timer.GetElapsedMilliseconds();
			return true;
		};
		tests.emplace_back(std::move(desc));
	}

	{
		TestDescription desc;
		desc.Name = "CopyIndividually";
		desc.Run = [](TestContext& context, D3D12_CPU_DESCRIPTOR_HANDLE descriptorDestination, float& executionTimeOut) {
			ComPtr<ID3D12DescriptorHeap> sourceHeap;
			if (!CreateDescriptorHeap(context.Device, HeapVisibility::NonShader, context.Textures.size(), sourceHeap))
			{
				return false;
			}
			CreateDescriptors(context.Device, context.Textures, sourceHeap->GetCPUDescriptorHandleForHeapStart());

			TestTimer timer;
			timer.Reset();

			for (size_t i = 0; i < context.Textures.size(); ++i)
			{
				context.Device->CopyDescriptorsSimple(1, descriptorDestination, sourceHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}

			timer.Stop();
			executionTimeOut = timer.GetElapsedMilliseconds();
			return true;
		};
		tests.emplace_back(std::move(desc));
	}

	{
		TestDescription desc;
		desc.Name = "CopyFromSeparateHeaps";
		desc.Run = [](TestContext& context, D3D12_CPU_DESCRIPTOR_HANDLE descriptorDestination, float& executionTimeOut) {
			std::vector<ComPtr<ID3D12DescriptorHeap>> sourceHeaps(context.Textures.size(), {});

			for (int i = 0; i < context.Textures.size(); ++i)
			{
				if (!CreateDescriptorHeap(context.Device, HeapVisibility::NonShader, 1, sourceHeaps[i]))
				{
					return false;
				}

				auto textureIterator = context.Textures.begin() + i;
				CreateDescriptors(context.Device, { textureIterator, textureIterator + 1 }, sourceHeaps[i]->GetCPUDescriptorHandleForHeapStart());
			}

			const UINT DESCRIPTOR_STRIDE = context.Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			TestTimer timer;
			timer.Reset();

			D3D12_CPU_DESCRIPTOR_HANDLE dest = descriptorDestination;
			for (size_t i = 0; i < context.Textures.size(); ++i)
			{
				context.Device->CopyDescriptorsSimple(1, dest, sourceHeaps[i]->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				dest.ptr += DESCRIPTOR_STRIDE;
			}

			timer.Stop();
			executionTimeOut = timer.GetElapsedMilliseconds();
			return true;
		};
		tests.emplace_back(std::move(desc));
	}

	{
		TestDescription desc;
		desc.Name = "CopyTogether";
		desc.Run = [](TestContext& context, D3D12_CPU_DESCRIPTOR_HANDLE descriptorDestination, float& executionTimeOut) {
			ComPtr<ID3D12DescriptorHeap> sourceHeap;
			if (!CreateDescriptorHeap(context.Device, HeapVisibility::NonShader, context.Textures.size(), sourceHeap))
			{
				return false;
			}
			CreateDescriptors(context.Device, context.Textures, sourceHeap->GetCPUDescriptorHandleForHeapStart());

			TestTimer timer;
			timer.Reset();

			context.Device->CopyDescriptorsSimple(context.Textures.size(), descriptorDestination, sourceHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			timer.Stop();
			executionTimeOut = timer.GetElapsedMilliseconds();
			return true;
		};
		tests.emplace_back(std::move(desc));
	}

	{
		TestDescription desc;
		desc.Name = "CreateAndCopyTogether";
		desc.Run = [](TestContext& context, D3D12_CPU_DESCRIPTOR_HANDLE descriptorDestination, float& executionTimeOut) {
			ComPtr<ID3D12DescriptorHeap> sourceHeap;
			if (!CreateDescriptorHeap(context.Device, HeapVisibility::NonShader, context.Textures.size(), sourceHeap))
			{
				return false;
			}

			TestTimer timer;
			timer.Reset();

			CreateDescriptors(context.Device, context.Textures, sourceHeap->GetCPUDescriptorHandleForHeapStart());
			context.Device->CopyDescriptorsSimple(context.Textures.size(), descriptorDestination, sourceHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			timer.Stop();
			executionTimeOut = timer.GetElapsedMilliseconds();
			return true;
			};
		tests.emplace_back(std::move(desc));
	}

	struct TestResult
	{
		float CPUExecutionTime = 0.0f;
	};

	std::vector<TestResult> results(tests.size(), {});

	const UINT numTextures = 10'000;
	
	for (size_t i = 0; i < tests.size(); ++i)
	{
		const TestDescription& test = tests[i];
		TestResult& result = results[i];

		std::cout << "Test: " << test.Name << std::endl;

		std::cout << "\tSetting up test..." << std::endl;

		TestContext context;
		if (!SetupTest(numTextures, context))
		{
			std::cout << "\tFailed. Skipping test." << std::endl;
			continue;
		}

		std::cout << "\tRunning test..." << std::endl;

		ComPtr<ID3D12DescriptorHeap> destinationHeap;
		ASSERT_TRUE(CreateDescriptorHeap(context.Device, HeapVisibility::Shader, numTextures, destinationHeap));
		ASSERT_TRUE(test.Run(context, destinationHeap->GetCPUDescriptorHandleForHeapStart(), result.CPUExecutionTime));
	}

	for (size_t i = 0; i < tests.size(); ++i)
	{
		const TestDescription& test = tests[i];
		const TestResult& result = results[i];

		std::cout << "Test '" << test.Name << "' completed in " << result.CPUExecutionTime << "ms" << std::endl;
	}
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

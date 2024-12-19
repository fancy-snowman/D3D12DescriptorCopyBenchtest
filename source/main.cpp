#include <gtest/gtest.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <iostream>

using namespace Microsoft::WRL;

// Helper function to initialize D3D12
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

// Test case to verify D3D12 initialization
TEST(D3D12Test, Initialization) {
    ComPtr<ID3D12Device> device;
    ComPtr<IDXGIFactory4> factory;

    ASSERT_TRUE(InitializeD3D12(device, factory));
    ASSERT_NE(device, nullptr);
    ASSERT_NE(factory, nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

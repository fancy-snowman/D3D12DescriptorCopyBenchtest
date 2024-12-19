-- premake5.lua

BASE_DIR = path.getabsolute(".")

BUILD_DIR = BASE_DIR .. "/build"
BIN_DIR = BUILD_DIR .. "/bin/%{cfg.buildcfg}"
LIB_DIR = BUILD_DIR .. "/lib/%{cfg.buildcfg}"
OBJ_DIR = BUILD_DIR .. "/obj/%{cfg.buildcfg}"
VS_DIR = BUILD_DIR .. "/vs"

THIRDPARTY_DIR = BASE_DIR .. "/thirdparty"
GTEST_DIR = THIRDPARTY_DIR .. "/googletest"

-- Helper function to download Google Test if not present
function DownloadGoogleTest()
    if not os.isdir(GTEST_DIR) then
        print("Cloning Google Test repository...")
        os.execute("git clone --depth 1 https://github.com/google/googletest.git " .. GTEST_DIR)
    else
        print("Google Test already exists. Skipping clone.")
    end
end

-- Call the function to download GTest
DownloadGoogleTest()

workspace "D3D12_Benchmark"
    configurations { "Debug", "Release" }
    startproject "Benchmark"

    architecture "x64"
    location (VS_DIR)

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter {}

project "GTestLib"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    targetdir (LIB_DIR)
    objdir (OBJ_DIR)

    files {
        GTEST_DIR .. "/googletest/src/**.cc",
        GTEST_DIR .. "/googletest/include/**.h"
    }

    includedirs {
        GTEST_DIR .. "/googletest/",
        GTEST_DIR .. "/googletest/include"
    }

project "Benchmark"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir (BIN_DIR)
    objdir (OBJ_DIR)

    files {
        "soruce/**.h",
        "source/**.cpp"
    }

    includedirs {
        "source/Benchmark",
        GTEST_DIR .. "/googletest/include",
        "$(DXSDK_DIR)/Include" -- Assumes the DirectX SDK is installed
    }

    libdirs {
        LIB_DIR,
        "$(DXSDK_DIR)/Lib/x64"
    }

    links {
        "GTestLib",
        "d3d12",
        "dxgi",
        "d3dcompiler"
    }

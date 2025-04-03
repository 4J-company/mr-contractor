# Enable CPM.cmake
include(FetchContent)
set(CPM_DOWNLOAD_VERSION 0.38.5)
file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
     ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# Platform-specific settings
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/MP /permissive- /Zc:__cplusplus)
elseif(APPLE)
    add_compile_options(-fobjc-arc)
    find_library(CORESERVICES CoreServices)
else()
    add_compile_options(-fPIC)
endif()

# Cross-platform dependencies
CPMFindPackage(
  NAME benchmark
  GITHUB_REPOSITORY google/benchmark
  GIT_TAG main
  OPTIONS
    "BENCHMARK_ENABLE_TESTING OFF"
)

CPMAddPackage("gh:google/cpu_features#main")

# Platform-specific compilation flags
add_library(platform_config INTERFACE)
target_compile_features(platform_config INTERFACE cxx_std_20)

if (MSVC)
  target_compile_options(platform_config INTERFACE
     /Ob2 /DNDEBUG /Zc:inline
  )
else()
  target_compile_options(platform_config INTERFACE
    -O3
    -march=native
    -fno-omit-frame-pointer
  )
endif()

set(MR_CONTRACTOR_BENCH_DEPS
  benchmark
  platform_config
  ${CORESERVICES}
)

file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.2/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:4J-company/work_contract#main")

if (NOT TARGET vtll)
  file(
    DOWNLOAD
    https://raw.githubusercontent.com/hlavacs/ViennaTypeListLibrary/refs/heads/main/include/VTLL.h
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/vtll-src/vtll/vtll.hpp
    EXPECTED_HASH SHA256=0446e7cde3e3a4f1289d8db630e9c3bb068d1a66356a3f3747b1777d9a4d7268
  )
  add_library(vtll INTERFACE "")
  target_include_directories(vtll INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/_deps/vtll-src/)
endif()

if (MR_CONTRACTOR_ENABLE_BENCHMARK)
  CPMFindPackage(
    NAME benchmark
    GITHUB_REPOSITORY google/benchmark
    GIT_TAG main
    OPTIONS
      "BENCHMARK_ENABLE_TESTING OFF"
  )
endif()

if (benchmark_ADDED)
  # patch benchmark target
  set_target_properties(benchmark PROPERTIES CXX_STANDARD 17)
endif()

if (MR_CONTRACTOR_ENABLE_TESTING)
  CPMFindPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG main
    OPTIONS
      "INSTALL_GTEST OFF"
      "gtest_force_shared_crt ON"
  )
endif()


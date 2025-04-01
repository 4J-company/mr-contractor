file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.2/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:4J-company/work_contract#main")
CPMAddPackage("gh:4J-company/function47#master")

if (NOT TARGET mp)
  file(
    DOWNLOAD
    https://raw.githubusercontent.com/qlibs/mp/main/mp
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/mp-src/mp/mp
  )
  add_library(mp INTERFACE "")
  target_include_directories(mp INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/_deps/mp-src/)
endif()

if (MR_CONTRACTOR_ENABLE_BENCHMARK)
  include(cmake/benchdeps.cmake)
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


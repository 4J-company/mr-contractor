cmake_minimum_required(VERSION 3.27)

# variables
set(MR_CONTRACTOR_PROJECT_NAME mr-contractor)
set(MR_CONTRACTOR_LIB_NAME mr-contractor-lib)
set(MR_CONTRACTOR_BENCH_NAME mr-contractor-bench)
set(MR_CONTRACTOR_TESTS_NAME mr-contractor-tests)

# options
option(MR_CONTRACTOR_ENABLE_BENCHMARK "Whether benchmarks are downloaded and built" OFF)
option(MR_CONTRACTOR_ENABLE_TESTING   "Whether tests are downloaded and built" OFF)

project(
  ${MR_CONTRACTOR_PROJECT_NAME}
  VERSION 1.0.0
)

include(cmake/dependencies.cmake)

# setup library
add_library(${MR_CONTRACTOR_LIB_NAME} INTERFACE
  include/mr-contractor/executor.hpp
  include/mr-contractor/par.hpp
  include/mr-contractor/seq.hpp
  include/mr-contractor/contractor.hpp
)
target_include_directories(${MR_CONTRACTOR_LIB_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${MR_CONTRACTOR_LIB_NAME} INTERFACE work_contract vtll)
target_compile_features(${MR_CONTRACTOR_LIB_NAME} INTERFACE cxx_std_23)

# tests
if (MR_CONTRACTOR_ENABLE_TESTING)
  add_executable(${MR_CONTRACTOR_TESTS_NAME} tests/main.cpp)
  target_link_libraries(${MR_CONTRACTOR_TESTS_NAME} PRIVATE ${MR_CONTRACTOR_LIB_NAME} gtest_main)
endif ()

# bench
if (MR_CONTRACTOR_ENABLE_BENCHMARK)
  # add_executable(${MR_CONTRACTOR_BENCH_NAME} bench/main.cpp)
  # target_link_libraries(${MR_CONTRACTOR_BENCH_NAME} PRIVATE ${MR_CONTRACTOR_LIB_NAME} benchmark)
endif ()

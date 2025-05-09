#include <benchmark/benchmark.h>
#include <mr-contractor/contractor.hpp>
#include <chrono>

#include "maps.hpp"

// ================= Platform-Neutral Timing =================
using Clock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<Clock>;
using duration = std::chrono::duration<double>;

// ================= Benchmark Configuration =================
constexpr size_t kMinThreads = 1;
constexpr size_t kMaxThreads = 32;

// for measuring overhead of task nesting depth (DAG is perfect binary tree)
inline static TaskMap nested_task_map = create_nested_task_map();
// for measuring overhead of number of stages inside a task (no nested tasks)
inline static TaskMap flat_task_map = create_flat_task_map();

void BM_NestedTasks(benchmark::State& state) {
  auto &task = nested_task_map[state.range(0)];
  for(auto _ : state) {
    auto x = task->execute().result();
    benchmark::DoNotOptimize(x);
  }
}
BENCHMARK(BM_NestedTasks)
  ->RangeMultiplier(2)
  ->Range(1, 128)
  ->Unit(benchmark::kMillisecond);
  ->Complexity()
;

void BM_FlatTasks(benchmark::State& state) {
  auto &task = nested_task_map[state.range(0)];
  for(auto _ : state) {
    auto x = task->execute().result();
    benchmark::DoNotOptimize(x);
  }
}
BENCHMARK(BM_FlatTasks)
  ->RangeMultiplier(2)
  ->Range(1, 128)
  ->Unit(benchmark::kMillisecond);
  ->Complexity()
;

// ================= Main Function =================
int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}

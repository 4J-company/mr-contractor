#include <benchmark/benchmark.h>
#include <mr-contractor/contractor.hpp>
#include <vector>
#include <atomic>
#include <thread>

// ================= Platform-Neutral Timing =================
using Clock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<Clock>;
using duration = std::chrono::duration<double>;

// ================= Benchmark Configuration =================
constexpr size_t kMinThreads = 1;
constexpr size_t kMaxThreads = 32;
constexpr size_t kTasksPerOp = 1'000'000;
constexpr size_t kMemoryChunkSize = 4096; // 4KB pages

// ================= Task Systems Under Test =================
class MrContractorSystem {
  public:
    explicit MrContractorSystem(size_t threads) {
      mr::Executor::get().threads.resize(threads);
    }

    template<typename F>
      void execute_bulk(F&& func, size_t tasks) {
        auto prototype = mr::Sequence{std::forward<F>(func)};
        auto task = mr::apply(prototype, tasks);
        task->execute();
      }
};

// ================= Benchmark Suite =================
template<typename TaskSystem>
class TaskSystemBenchmark : public benchmark::Fixture {
  protected:
    void SetUp(const benchmark::State& state) override {
      system_ = std::make_unique<TaskSystem>(state.range(0));
    }

    void TearDown(const benchmark::State&) override {
      system_.reset();
    }

    std::unique_ptr<TaskSystem> system_;
    std::atomic<size_t> counter_{0};
};

// Throughput benchmark
BENCHMARK_TEMPLATE_DEFINE_F(TaskSystemBenchmark, Throughput, MrContractorSystem)
  (benchmark::State& state) {
    const size_t tasks = state.range(1);
    const size_t work_size = state.range(2);

    for (auto _ : state) {
      counter_.store(0, std::memory_order_relaxed);

      system_->execute_bulk([&, work_size] (int) -> int {
        // Simulate memory-bound workload
        volatile char buffer[kMemoryChunkSize];
        for(size_t i = 0; i < work_size; ++i) {
          buffer[i % kMemoryChunkSize] = static_cast<char>(i);
        }
        counter_.fetch_add(1, std::memory_order_relaxed);
        return 47;
      }, tasks);

      benchmark::DoNotOptimize(counter_.load());
    }

    state.counters["tasks/s"] = benchmark::Counter(
        tasks, benchmark::Counter::kIsRate);
    state.counters["bytes/op"] =
      benchmark::Counter(work_size * kMemoryChunkSize);
  }

// Scaling efficiency analysis
BENCHMARK_TEMPLATE_DEFINE_F(TaskSystemBenchmark, Scaling, MrContractorSystem)
  (benchmark::State& state) {
    const size_t base_threads = 1;
    const size_t tasks = state.range(1);
    double single_thread_time = 0;

    // Measure single-thread baseline
    {
      MrContractorSystem base_system(base_threads);
      auto start = Clock::now();
      base_system.execute_bulk([] (int) mutable -> int { return 47; }, tasks);
      single_thread_time = duration(Clock::now() - start).count();
    }

    for (auto _ : state) {
      auto start = Clock::now();
      system_->execute_bulk([] (int) mutable -> int { return 47; }, tasks);
      auto elapsed = duration(Clock::now() - start).count();

      const double speedup = single_thread_time / elapsed;
      const double efficiency = speedup / state.range(0);
      state.counters["efficiency"] = efficiency;
    }
  }

// ================= Benchmark Registration =================
BENCHMARK_REGISTER_F(TaskSystemBenchmark, Throughput)
  ->ArgsProduct({
      {kMinThreads, kMaxThreads},    // Thread count
      {kTasksPerOp},                 // Task count
      {1, 10, 100}                   // Work size (memory chunks)
      })
->Repetitions(3)
  ->ComputeStatistics("min", [](const std::vector<double>& v) mutable {
      return *std::min_element(v.begin(), v.end());
      })
->ComputeStatistics("max", [](const std::vector<double>& v) mutable {
    return *std::max_element(v.begin(), v.end());
    });

BENCHMARK_REGISTER_F(TaskSystemBenchmark, Scaling)
  ->ArgsProduct({
      benchmark::CreateDenseRange(2, 32, 2),  // Thread count
      {kTasksPerOp}                           // Task count
      });

// ================= Main Function =================
int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}

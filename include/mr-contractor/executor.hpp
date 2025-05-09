#pragma once

#include <vector>
#include <thread>

#include "def.hpp"

namespace mr {
  struct Executor {
  public:
    inline static int threadcount = std::thread::hardware_concurrency();

    bcpp::work_contract_group group;
    std::vector<std::jthread> threads;

    static Executor & get() noexcept {
      static Executor executor {};
      return executor;
    }

    void thread_count(int n) {
      if (n != thread_count()) {
        resize(n);
      }
    }

    int thread_count() const {
      return threads.size();
    }

  private:
    void resize(int n) {
      threads.clear();
      threads.resize(n);
      for (int i = 0; i < n; i++) {
        threads[i] = std::jthread(
          [this](const auto &token) {
            while (not token.stop_requested()) {
              group.execute_next_contract();
            }
          }
        );
      }
    }

    Executor() noexcept {
      resize(threadcount);
    }
  };
}


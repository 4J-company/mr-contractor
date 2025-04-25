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

    void set_thread_count(int n) {
      resize(n);
    }

  private:
    void resize(int n) {
      for (int i = 0; i < n; i++) {
        threads.emplace_back(
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


#pragma once

#include <vector>
#include <thread>

#include "def.hpp"

namespace mr {
  struct Executor {
  public:
    inline static constexpr int threadcount = 8;

    bcpp::work_contract_group group;
    std::vector<std::jthread> threads;

    static Executor & get() noexcept {
      static Executor executor {};
      return executor;
    }

  private:
    Executor() noexcept {
      for (int i = 0; i < threadcount; i++) {
        threads.emplace_back(
          [this](const auto &token) {
            while (not token.stop_requested()) {
              group.execute_next_contract();
            }
          }
        );
      }
    }
  };
}


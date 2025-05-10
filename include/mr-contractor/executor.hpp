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
    std::mutex resize_mutex;

    static Executor & get() noexcept {
      static Executor executor {};
      return executor;
    }

    void thread_count(int n) {
      assert(n > 0);

      if (n == thread_count()) {
        return;
      }

      resize(std::clamp(n, 1, threadcount));
    }

    int thread_count() const {
      return threads.size();
    }

  private:

    void resize(int n) {
      std::lock_guard l(resize_mutex);

      auto worker = [this](const std::stop_token &token) {
        while (not token.stop_requested()) {
          group.execute_next_contract();
        }
      };

      if (n < threads.size()) {
        for (int i = n; i < threads.size(); i++) {
          threads[i].request_stop();
        }
        threads.resize(n);
      }
      else {
        for (int i = threads.size(); i < n; i++) {
          threads.emplace_back(worker);
        }
      }
    }

    Executor() noexcept {
      resize(threadcount);
    }
  };
}


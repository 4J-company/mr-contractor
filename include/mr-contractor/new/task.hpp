#pragma once

#include <atomic>
#include <barrier>
#include <cstddef>
#include <vector>
#include <memory>

#include "mr-contractor/def.hpp"
#include "mr-contractor/executor.hpp"

namespace mr::detail {
  template <typename ResultT>
    struct TaskBase {
      std::vector<Contract> contracts {};
      std::atomic_flag completion_flag = false;

      TaskBase() = default;
      virtual ~TaskBase() = default;

      TaskBase(const TaskBase&) = delete;
      TaskBase& operator=(const TaskBase&) = delete;
      TaskBase(TaskBase&& other) {
        contracts = std::move(other.contracts);
        if (other.completion_flag.test()) {
          completion_flag.test_and_set();
        } else {
          completion_flag.clear();
        }
      }
      TaskBase& operator=(TaskBase&& other) {
        contracts = std::move(other.contracts);
        if (other.completion_flag.test()) {
          completion_flag.test_and_set();
        } else {
          completion_flag.clear();
        }
        return *this;
      }

      virtual TaskBase & schedule() {
        update_object();
        contracts.front().schedule();
        return *this;
      }

      virtual TaskBase & wait() {
        completion_flag.wait(false);
        return *this;
      }

      TaskBase & execute() {
        return schedule().wait();
      }

      virtual void update_object() = 0;

      [[nodiscard]] virtual ResultT result() = 0;
    };

  template <typename VariantT, typename ResultT>
    struct SeqTaskImpl : TaskBase<ResultT> {
      VariantT _initial;
      std::unique_ptr<VariantT> object;

      SeqTaskImpl() = default;
      ~SeqTaskImpl() override = default;

      SeqTaskImpl(const SeqTaskImpl&) = delete;
      SeqTaskImpl& operator=(const SeqTaskImpl&) = delete;
      SeqTaskImpl(SeqTaskImpl&&) = default;
      SeqTaskImpl& operator=(SeqTaskImpl&&)  = default;

      SeqTaskImpl(VariantT &&initial)
        : _initial(initial)
      {}

      void update_object() override final {
        object = std::make_unique<VariantT>(_initial);
      }

      [[nodiscard]] ResultT result() override final {
        auto &v = *object.get();
        auto &r = std::get<ResultT>(v);

        return std::move(r);
      }
    };

  template <typename T> constexpr bool is_seq_task_impl = false;
  template <typename I, typename R> constexpr bool is_seq_task_impl<SeqTaskImpl<I, R>> = true;
  template <typename T> concept SeqTaskImplInstance = is_seq_task_impl<T>;

  template <size_t TaskNumber, typename InputT, typename ResultT>
    struct ParTaskImpl : TaskBase<ResultT> {
      InputT _initial;
      std::unique_ptr<ResultT> object = std::make_unique<ResultT>();
      std::barrier<std::function<void()>> barrier {TaskNumber + 1}; // NOTE: +1 to account for the calling thread waiting on this

      ParTaskImpl() = default;
      ~ParTaskImpl() override = default;

      ParTaskImpl(const ParTaskImpl &) = delete;
      ParTaskImpl & operator=(const ParTaskImpl &) = delete;
      ParTaskImpl(ParTaskImpl &&) = default;
      ParTaskImpl & operator=(ParTaskImpl &&) = default;

      ParTaskImpl(InputT &&initial)
        : _initial(initial)
      {}

      void update_object() override final {
      }

      TaskBase<ResultT> & wait() override final {
        printf("life is shit2\n");
        barrier.arrive_and_wait();
        printf("life is shit3\n");
        return *this;
      }

      TaskBase<ResultT> & schedule() override final {
        printf("life is shit1\n");
        update_object();
        for (auto &c : TaskBase<ResultT>::contracts) {
          c.schedule();
        }
        return *this;
      }

      [[nodiscard]] ResultT result() override final {
        ResultT &r = *object.get();
        return std::move(r);
      }
    };

  template <typename T> constexpr bool is_par_task_impl = false;
  template <size_t S, typename ...Ts> constexpr bool is_par_task_impl<ParTaskImpl<S, Ts...>> = true;
  template <typename T> concept ParTaskImplInstance = is_par_task_impl<T>;
}

namespace mr {
  template <typename ResultT> using Task = std::unique_ptr<detail::TaskBase<ResultT>>;
}

#pragma once

#include <atomic>
#include <barrier>
#include <cstddef>
#include <vector>
#include <memory>

#include "mr-contractor/def.hpp"
#include "traits.hpp"

namespace mr::detail {
  // TODO:
  //    - introduce `DeferredTask`, which takes `getter` instead of `initial`
  //        - make `NestedTaskT` concept which is Deferred<Par/Seq>Task
  template <typename ResultT>
    struct TaskBase {
      TaskBase() = default;
      virtual ~TaskBase() = default;

      // customization points
      [[nodiscard]]
      virtual ResultT    result()        = 0;
      virtual void       update_object() = 0;
      virtual TaskBase & schedule()      = 0;
      virtual TaskBase & wait()          = 0;

      TaskBase & execute() {
        return schedule().wait();
      }
    };

  // `NumOfTasks` must be a std::integral_constant instance to make `mr::InstanceOf<SeqTaskImpl>` work
  template <typename NumOfTasks, typename VariantT, typename InputT, typename ResultT>
    struct SeqTaskImpl : TaskBase<ResultT> {
      static constexpr auto size = NumOfTasks::value;

      InputT _initial;

      FunctionWrapper<InputT(void)> _getter = [this]() -> InputT { return _initial; };
      std::unique_ptr<VariantT> _object;

      std::array<Contract, size> contracts {};

      std::atomic_flag completion_flag{};

      SeqTaskImpl() = default;
      ~SeqTaskImpl() override = default;

      SeqTaskImpl(InputT initial)
        : _initial(std::move(initial))
      {}

      SeqTaskImpl(FunctionWrapper<InputT()> getter)
        : _getter(std::move(getter))
      {}

      SeqTaskImpl(const SeqTaskImpl&) = delete;
      SeqTaskImpl& operator=(const SeqTaskImpl&) = delete;

      SeqTaskImpl(SeqTaskImpl&& other) noexcept
        : _initial(std::move(other._initial))
        , _getter(std::move(other._getter))
        , _object(std::move(other._object))
        , contracts(std::move(other.contracts))
        , completion_flag(other.completion_flag.test()) {
          other.completion_flag.clear();
      }
      SeqTaskImpl& operator=(SeqTaskImpl&& other) noexcept {
        if (this != &other) {
          _initial = std::move(other._initial);
          _getter = std::move(other._getter);
          _object = std::move(other._object);
          contracts = std::move(other.contracts);
          if (other.completion_flag.test()) {
            completion_flag.test_and_set();
          }
          else {
            completion_flag.clear();
          }
          other.completion_flag.clear();
        }
        return *this;
      }

      void update_object() override final {
        _object = std::make_unique<VariantT>(_getter());
      }

      TaskBase<ResultT> & schedule() override final {
        update_object();
        this->contracts.front().schedule();
        return *this;
      }

      TaskBase<ResultT> & wait() override final {
        this->completion_flag.wait(false);
        return *this;
      }

      [[nodiscard]] ResultT result() override final {
        auto &v = *_object.get();
        auto &r = std::get<ResultT>(v);

        return r;
      }
    };

  // `NumOfTasks` must be a std::integral_constant instance to make `mr::InstanceOf<ParTaskImpl>` work
  template <typename NumOfTasks, typename InputT, typename ResultT>
    struct ParTaskImpl : TaskBase<ResultT> {
      static constexpr auto size = NumOfTasks::value;

      InputT _initial;

      FunctionWrapper<InputT(void)> _getter = [this]() -> InputT { return _initial; };
      FunctionWrapper<void(void) noexcept> _on_finish = []() noexcept {};

      std::array<Contract, size> contracts {};

      InputT _input;
      std::unique_ptr<ResultT> _object = std::make_unique<ResultT>();
      std::barrier<FunctionView<void() noexcept>> _barrier {
        size + 1, // NOTE: +1 to account for the calling thread waiting on this
        FunctionView<void(void) noexcept>(_on_finish)
      };

      ParTaskImpl() = default;
      ~ParTaskImpl() override = default;

      ParTaskImpl(const ParTaskImpl &) = delete;
      ParTaskImpl & operator=(const ParTaskImpl &) = delete;
      ParTaskImpl(ParTaskImpl &&) = default;
      ParTaskImpl & operator=(ParTaskImpl &&) = default;

      ParTaskImpl(InputT initial)
        : _initial(std::move(initial))
      {}

      ParTaskImpl(FunctionWrapper<InputT(void)> getter)
        : _getter(std::move(getter))
      {}

      void update_object() override final {
        _input = _getter();
      }

      TaskBase<ResultT> & wait() override final {
        _barrier.arrive_and_wait();
        return *this;
      }

      TaskBase<ResultT> & schedule() override final {
        update_object();
        for (auto &c : contracts) {
          c.schedule();
        }
        return *this;
      }

      [[nodiscard]] ResultT result() override final {
        ResultT &r = *_object.get();
        return r;
      }
    };
}

namespace mr {
  // TODO: make `Task` `Awaitable`
  template <typename ResultT> using Task = std::unique_ptr<detail::TaskBase<ResultT>>;
}

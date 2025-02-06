#pragma once

#include <atomic>
#include <cassert>
#include <functional>
#include <vector>

#include <vtll/vtll.hpp>

#include "mr-contractor/executor.hpp"

namespace mr {
  // function to specialize
  template <typename ResultT, typename ...Args> auto make_pipe_prototype();

  template <typename ...> struct PipePrototype;

  template <typename ResultT>
  struct PipeBase {
    std::vector<Contract> contracts {};
    std::atomic_flag completion_flag = false;

    PipeBase() noexcept = default;
    virtual ~PipeBase() noexcept = default;

    PipeBase(const PipeBase&) = delete;
    PipeBase& operator=(const PipeBase&) = delete;

    PipeBase(PipeBase&& other) noexcept {
      contracts = std::move(other.contracts);
      if (other.completion_flag.test()) {
        completion_flag.test_and_set();
      } else {
        completion_flag.clear();
      }
    }
    PipeBase& operator=(PipeBase&& other) noexcept {
      contracts = std::move(other.contracts);
      if (other.completion_flag.test()) {
        completion_flag.test_and_set();
      } else {
        completion_flag.clear();
      }
      return *this;
    }

    PipeBase & schedule() noexcept {
      update_object();
      contracts.front().schedule();
      return *this;
    }

    PipeBase & wait() noexcept {
      completion_flag.wait(false);
      return *this;
    }

    PipeBase & execute() noexcept {
      return schedule().wait();
    }

    virtual void update_object() noexcept = 0;

    [[nodiscard]] virtual ResultT result() = 0;
  };

  template <typename ResultT> using PipeHandle = std::unique_ptr<PipeBase<ResultT>>;

  template <typename VariantT, typename ResultT>
  struct Pipe : PipeBase<ResultT> {
    VariantT _initial;
    std::unique_ptr<VariantT> object;

    Pipe() = default;
    ~Pipe() noexcept override = default;

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    Pipe(Pipe&&) = default;
    Pipe& operator=(Pipe&&) = default;

    Pipe(VariantT &&initial)
      : _initial(initial)
    {}

    virtual void update_object() noexcept override final {
      object = std::make_unique<VariantT>(_initial);
    }

    [[nodiscard]] ResultT result() override final {
      auto &v = *object.get();
      auto &r = std::get<ResultT>(v);

      return std::move(r);
    }

    template<size_t StageIdx, size_t EndIdx, typename InputT, typename OutputT>
      void create_contract(const std::function<OutputT(InputT)> &stage) noexcept {
        auto contract = Executor::get().group.create_contract(
          [this, stage]() {
            auto result = stage(std::move(std::get<InputT>(*object.get())));
            object->template emplace<OutputT>(std::move(result));

            if constexpr (StageIdx + 1 < EndIdx) {
              PipeBase<ResultT>::contracts[StageIdx + 1].schedule();
            } else {
              PipeBase<ResultT>::completion_flag.test_and_set(std::memory_order_release);
              PipeBase<ResultT>::completion_flag.notify_one();
            }
          }
        );
        PipeBase<ResultT>::contracts.emplace_back(std::move(contract));
      }

    template<size_t StageIdx, typename InputT, typename OutputT>
      void create_contract_unchecked(const std::function<OutputT(InputT)> &stage) noexcept {
        auto contract = Executor::get().group.create_contract(
          [this, stage]() {
            auto result = stage(std::move(std::get<InputT>(*object.get())));
            object->template emplace<OutputT>(std::move(result));
            PipeBase<ResultT>::contracts[StageIdx + 1].schedule();
          }
        );
        PipeBase<ResultT>::contracts.emplace_back(std::move(contract));
      }

    void create_contract(const std::function<void(void)> &stage) noexcept {
      auto contract = Executor::get().group.create_contract(
        [this, stage]() {
          stage();
          PipeBase<ResultT>::completion_flag.test_and_set(std::memory_order_release);
          PipeBase<ResultT>::completion_flag.notify_one();
        }
      );

      PipeBase<ResultT>::contracts.emplace_back(std::move(contract));
    }
  };

  template <typename ...Is, typename ...Os>
    struct PipePrototype<vtll::type_list<Is...>, vtll::type_list<Os...>> {

      using InputsListT = vtll::type_list<Is...>;
      using OutputsListT = vtll::type_list<Os...>;
      using InputOutputListT = vtll::type_list<Is..., Os...>;

      static_assert(vtll::is_same_list<
          vtll::erase_Nth<InputsListT, 0>,
          vtll::erase_Nth<OutputsListT, vtll::size<OutputsListT>::value - 1>
        >::value, "Invalid transform chain (type mismatch)");

      using InitialInputT = vtll::front<InputsListT>;
      using FinalOutputT = vtll::back<OutputsListT>;

      using UniqueInputOutputListT = vtll::remove_duplicates<InputOutputListT>;
      using VariantT = vtll::to_variant<UniqueInputOutputListT>;
      using PipeT = Pipe<VariantT, FinalOutputT>;
      using PipeHandleT = PipeHandle<FinalOutputT>;

      std::tuple<std::function<Os(Is)>...> callables;

      PipePrototype() = default;
      PipePrototype(const PipePrototype&) = default;
      PipePrototype & operator=(const PipePrototype&) = default;
      PipePrototype(PipePrototype&&) = default;
      PipePrototype & operator=(PipePrototype&&) = default;

      PipePrototype(std::function<Os(Is)> ...cs) : callables(cs...) {}

      constexpr PipeHandleT on(InitialInputT &&initial) {
        constexpr size_t EndIdx = sizeof...(Os);

        std::unique_ptr<PipeT> pipe = std::make_unique<PipeT>(std::move(initial));
        [this, &pipe]<size_t... Indices>(std::index_sequence<Indices...>) {
          (pipe->template create_contract<Indices, EndIdx>(std::get<Indices>(callables)), ...);
        }(std::make_index_sequence<EndIdx>{});

        return pipe;
      }

      // appends `on_finish` function as the last contract
      PipeHandleT on(InitialInputT &&initial, std::function<void(void)> on_finish) {
        constexpr size_t EndIdx = sizeof...(Os);

        std::unique_ptr<PipeT> pipe = std::make_unique<PipeT>(std::move(initial));
        [this, &pipe]<size_t... Indices>(std::index_sequence<Indices...>) {
          (pipe->template create_contract_unchecked<Indices>(std::get<Indices>(callables)), ...);
        }(std::make_index_sequence<EndIdx>{});

        pipe->create_contract(on_finish);

        return pipe;
      }
    };

    template <typename ...Is, typename ...Os>
      PipePrototype(std::function<Os(Is)> ...) -> PipePrototype<vtll::type_list<Is...>, vtll::type_list<Os...>>;
}

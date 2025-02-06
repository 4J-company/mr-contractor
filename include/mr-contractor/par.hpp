#pragma once

#include <barrier>

#include <vtll/vtll.hpp>

#include "mr-contractor/executor.hpp"

namespace mr {
  template <typename...> struct ParallelPipePrototype {};

  // std::tuple<InputsT...> inputs;
  // std::tuple<OutputsT...> outputs;

  template <typename InputTupleT, typename OutputTupleT>
    struct ParallelPipe {
      inline static constexpr size_t size = std::tuple_size_v<InputTupleT>;

      InputTupleT input;
      OutputTupleT output;
      std::array<Contract, size> contracts;
      std::barrier<> barrier {size + 1, []() noexcept {}};

      ParallelPipe() noexcept = default;
      ParallelPipe(const ParallelPipe&) = default;
      ParallelPipe & operator=(const ParallelPipe&) = default;
      ParallelPipe(ParallelPipe&&) = default;
      ParallelPipe & operator=(ParallelPipe&&) = default;

      ParallelPipe(InputTupleT &&initial) noexcept : input(initial) {}

      void wait() {
        barrier.arrive_and_wait();
      }

      template <size_t Index, typename InputT, typename OutputT>
        void create_contract(std::function<OutputT(InputT)> callable) {
          auto contract = Executor::get().group.create_contract(
            [this, callable]() {
              std::get<Index>(output) = callable(std::get<Index>(input));
              /* to suppress nodiscard warning */ [[maybe_unused]] auto _ = barrier.arrive();
            }
          );
          contracts[Index] = std::move(contract);
        }
    };

  template <typename ...Os, typename ...Is>
    struct ParallelPipePrototype<vtll::type_list<Is...>, vtll::type_list<Os...>> {
      using PipeT = ParallelPipe<std::tuple<Is...>, std::tuple<Os...>>;
      using PipeHandleT = std::unique_ptr<PipeT>;

      std::tuple<std::function<Os(Is)...>> callables;

      ParallelPipePrototype() = default;
      ParallelPipePrototype(const ParallelPipePrototype&) = default;
      ParallelPipePrototype & operator=(const ParallelPipePrototype&) = default;
      ParallelPipePrototype(ParallelPipePrototype&&) = default;
      ParallelPipePrototype & operator=(ParallelPipePrototype&&) = default;

      ParallelPipePrototype(std::function<Os(Is)> ...cs) : callables(cs...) {}

      PipeHandleT on(Is... inputs) {
        constexpr size_t EndIdx = sizeof...(Os);

        std::unique_ptr<PipeT> pipe = std::make_unique<PipeT>(std::forward_as_tuple(inputs...));
        [this, &pipe]<size_t... Indices>(std::index_sequence<Indices...>) {
          (pipe->template create_contract<Indices>(std::get<Indices>(callables)), ...);
        }(std::make_index_sequence<EndIdx>{});

        return pipe;
      }
    };

  template <typename ...Is, typename ...Os>
    ParallelPipePrototype(std::function<Os(Is)> ...) -> ParallelPipePrototype<vtll::type_list<Is...>, vtll::type_list<Os...>>;
}

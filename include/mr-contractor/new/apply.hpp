#pragma once

#include "stages.hpp"
#include "traits.hpp"
#include "task.hpp"

namespace mr::detail {
  template <size_t StageIdx, size_t EndIdx>
    void create_contract(SeqTaskImplInstance auto &task, const StageT auto &stage)
    {
      if constexpr (StageIdx + 1 < EndIdx) {
        create_contract_unchecked<StageIdx>(task, stage);
      } else {
        create_contract_final(task, std::get<StageIdx>(stage.stages));
      }
    }

  template <size_t StageIdx, size_t EndIdx>
    void create_contract(ParTaskImplInstance auto &task, const StageT auto &stage)
    {
      create_contract_unchecked<StageIdx>(task, stage);
    }

  template<size_t StageIdx, SequenceT SeqStageT>
    void create_contract_unchecked(typename SeqStageT::TaskImplT &task, const SeqStageT &stage)
    {
      using InputT = input_t<decltype(std::get<StageIdx>(stage.stages))>;
      using OutputT = output_t<decltype(std::get<StageIdx>(stage.stages))>;
      auto contract = Executor::get().group.create_contract(
        [&task, stage]() {
          auto result = std::get<StageIdx>(stage.stages)(std::move(std::get<InputT>(*task.object.get())));
          task.object->template emplace<OutputT>(std::move(result));
          task.contracts[StageIdx + 1].schedule();
        }
      );
      task.contracts.emplace_back(std::move(contract));
    }

  template<size_t StageIdx, ParallelT ParStageT>
    void create_contract_unchecked(typename ParStageT::TaskImplT &task, const ParStageT &stage)
    {
      using InputT = input_t<decltype(std::get<StageIdx>(stage.stages))>;
      using OutputT = output_t<decltype(std::get<StageIdx>(stage.stages))>;
      auto contract = Executor::get().group.create_contract(
        [&task, stage]() {
          auto result = std::get<StageIdx>(stage.stages)(std::get<StageIdx>(task._initial));
          std::get<StageIdx>(*task.object.get()) = std::move(result);
          [[maybe_unused]] auto _ = task.barrier.arrive(); /* to suppress nodiscard warning */
        }
      );
      task.contracts.emplace_back(std::move(contract));
    }

  template <typename InputT, typename OutputT>
  void create_contract_final(SeqTaskImplInstance auto &task, const std::function<OutputT(InputT)> &stage)
  {
    auto contract = Executor::get().group.create_contract(
      [&task, stage]() {
        if constexpr (std::is_same_v<OutputT(InputT), void(void)>) {
          stage();
        } else {
          auto result = stage(std::move(std::get<InputT>(*task.object.get())));
          task.object->template emplace<OutputT>(std::move(result));
        }
        task.completion_flag.test_and_set(std::memory_order_release);
        task.completion_flag.notify_one();
      }
    );
    task.contracts.emplace_back(std::move(contract));
  }

  void create_contract_final(ParTaskImplInstance auto &task, const std::function<void(void)> &stage)
  {
    // NOTE: cppreference says it's implementation defined whether CompletionFunc of a barrier is invoked
    task.barrier = {task.contracts.size(), [&task, stage]() {
      stage();
      task.completion_flag.test_and_set(std::memory_order_release);
      task.completion_flag.notify_one();
    }};
  }
};

namespace mr {
  template <typename T> concept ApplicableT = ParallelT<T> || SequenceT<T>;

  template <StageT ParSeqStageT>
    typename ParSeqStageT::TaskT apply(const ParSeqStageT &stage, typename ParSeqStageT::InputT initial)
    {
      using TaskImplT = ParSeqStageT::TaskImplT;
      using TaskT = ParSeqStageT::TaskT;

      auto res = std::make_unique<TaskImplT>(std::move(initial));

      constexpr auto EndIdx = std::tuple_size_v<decltype(stage.stages)>;

      // compile-time for loop
      [&stage, &res]<size_t... Indices>(std::index_sequence<Indices...>) {
        (create_contract<Indices, EndIdx>(*res.get(), stage), ...);
      }(std::make_index_sequence<EndIdx>{});

      return res;
    }

  template <StageT ParSeqStageT>
    typename ParSeqStageT::TaskT apply(const ParSeqStageT &stage, typename ParSeqStageT::InputT initial, std::function<void(void)> on_finish)
    {
      using TaskImplT = ParSeqStageT::TaskImplT;
      using TaskT = ParSeqStageT::TaskT;

      auto res = std::make_unique<TaskImplT>(std::move(initial));

      constexpr auto EndIdx = std::tuple_size_v<decltype(stage.stages)>;

      // compile-time for loop
      [&stage, &res]<size_t... Indices>(std::index_sequence<Indices...>) {
        (create_contract_unchecked<Indices>(*res.get(), stage), ...);
      }(std::make_index_sequence<EndIdx>{});

      create_contract_final(*res.get(), on_finish);

      return res;
    }
}

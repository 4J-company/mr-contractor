#pragma once

#include "mr-contractor/def.hpp"
#include "mr-contractor/executor.hpp"
#include "stages.hpp"
#include "traits.hpp"
#include "task.hpp"

namespace mr::detail {
  // size_t I: index of a Stage in target task
  //
  // Below are the following overloads:
  //   add(<Seq/Par>TaskImpl, <Func/Seq/Par>)

  // add(Seq, Func)
  template <size_t I, typename OutputT, typename InputT>
    inline void add(InstanceOf<SeqTaskImpl> auto &task, FunctionView<OutputT(InputT)> stage) {
      auto contract = Executor::get().group.create_contract(
        [&task, stage]() mutable {
          if constexpr (std::is_same_v<InputT, void>) {
            task._object->template emplace<OutputT>(stage());
          } else {
            task._object->template emplace<OutputT>(stage(std::move(std::get<InputT>(*task._object.get()))));
          }

          if constexpr (I < std::remove_reference_t<decltype(task)>::size - 1) {
            task.contracts[I + 1].schedule();
          } else {
            task.completion_flag.test_and_set(std::memory_order_release);
            task.completion_flag.notify_one();
          }
        }
      );

      task.contracts[I] = std::move(contract);
    }

  // add(Par, Func)
  template <size_t I, typename OutputT, typename InputT>
    inline void add(InstanceOf<ParTaskImpl> auto &task, FunctionView<OutputT(InputT)> stage) {
      auto contract = Executor::get().group.create_contract(
        [&task, stage]() mutable {
          std::get<I>(*task._object.get()) = stage(std::move(std::get<I>(task._input)));

          if constexpr (I < std::remove_reference_t<decltype(task)>::size - 1) {
            [[maybe_unused]] auto _ = task._barrier.arrive(); /* to suppress nodiscard warning */
          } else {
            task._barrier.arrive_and_wait();
          }
        }
      );

      task.contracts[I] = std::move(contract);
    }

// These functions are (maybe temporary) unnecessary.
// If any of them are not used in mr-importer, we will delete them
#if 0
  // add(Seq, <Par/Seq>)
  template <size_t I, Applicable StageT>
    inline void add(InstanceOf<SeqTaskImpl> auto &task, const StageT &stage) {
      using InputT = typename StageT::InputT;
      using OutputT = typename StageT::OutputT;

      FunctionWrapper<InputT(void)> getter = [&task]() {
        return std::get<InputT>(*task._object.get());
      };

      auto nested_task = apply(stage, std::move(getter));

      FunctionWrapper<OutputT(void)> callable =
        [nt = std::move(nested_task)]() {
          nt->execute();
          return nt->result();
        };

      add<I>(task, FunctionView<OutputT(void)>(callable));
    }

  // add(Par, <Par/Seq>)
  template <size_t I, Applicable StageT>
    inline void add(InstanceOf<ParTaskImpl> auto &task, const StageT &stage) {
      using InputT = typename StageT::InputT;
      using OutputT = typename StageT::OutputT;

      FunctionWrapper<InputT(void)> getter = [&task]() {
        return std::get<I>(*task._input);
      };

      auto nested_task = apply(stage, std::move(getter));

      FunctionWrapper<OutputT(void)> callable =
        [nt = std::move(nested_task) ]() {
          nt->execute();
          return nt->result();
        };

      add<I>(task, std::move(callable));
    }

  // add(Seq, StageRef)
  template <size_t I, ApplicableRef StageRefT>
    inline void add(InstanceOf<SeqTaskImpl> auto &task, StageRefT stage) {
      using InternalT = typename StageRefT::type;

      using InputT = input_t<InternalT>;
      using OutputT = output_t<InternalT>;

      FunctionWrapper<InputT(void)> getter = [&task]() {
        return std::get<InputT>(*task._object.get());
      };

      auto nested_task = apply(stage.get(), std::move(getter));
      FunctionWrapper<OutputT(void)> callable =
        [nt = std::move(nested_task)]() {
          nt->execute();
          return nt->result();
        };

      add<I>(task, FunctionView<OutputT(void)>(callable));
    }

  // add(Par, StageRef)
  template <size_t I, ApplicableRef StageRefT>
    inline void add(InstanceOf<ParTaskImpl> auto &task, StageRefT stage) {
      using InternalT = typename StageRefT::type;

      using InputT = input_t<InternalT>;
      using OutputT = output_t<InternalT>;

      FunctionWrapper<InputT(void)> getter = [&task]() {
        return std::get<I>(*task._input);
      };

      auto nested_task = apply(stage.get(), std::move(getter));
      FunctionWrapper<OutputT(void)> callable =
        [nt = std::move(nested_task)]() {
          nt->execute();
          return nt->result();
        };

      add<I>(task, FunctionView<OutputT(void)>(callable));
    }
#endif
}

namespace mr {
  // TODO(dk6): use Applicable instead Stage
  template <Stage StageT>
    typename StageT::TaskT apply(const StageT &stage, FunctionWrapper<typename StageT::InputT(void)> &&getter) {
      using TaskImplT = StageT::TaskImplT;
      using TaskT = StageT::TaskT;

      auto task = std::make_unique<TaskImplT>(std::move(getter));
      [&task, &stage]<size_t ...Is>(std::index_sequence<Is...>) {
        (detail::add<Is>(*task.get(), detail::to_wrapper_view_v(std::get<Is>(stage.stages))), ...);
      }(std::make_index_sequence<StageT::sub_stages_count>());

      return task;
    }

  template <Stage StageT>
    typename StageT::TaskT apply(const StageT& stage, typename StageT::InputT initial) {
      using TaskImplT = StageT::TaskImplT;
      using TaskT = StageT::TaskT;

      auto task = std::make_unique<TaskImplT>(std::move(initial));
      [&task, &stage]<size_t ...Is>(std::index_sequence<Is...>) {
        (detail::add<Is>(*task.get(), detail::to_wrapper_view_v(std::get<Is>(stage.stages))), ...);
      }(std::make_index_sequence<StageT::sub_stages_count>());

      return task;
    }
}


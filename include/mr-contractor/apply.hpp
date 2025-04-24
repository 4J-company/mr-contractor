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
    inline void add(SeqTaskImplInstance auto &task, FunctionView<OutputT(InputT)> stage) {
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
    inline void add(ParTaskImplInstance auto &task, FunctionView<OutputT(InputT)> stage) {
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
  // TODO(dk6): rename T to StageT after concepts names refactoring
  template <size_t I, ApplicableT T>
    inline void add(SeqTaskImplInstance auto &task, const T &stage) {
      using InputT = typename T::InputT;
      using OutputT = typename T::OutputT;

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
  template <size_t I, ApplicableT T>
    inline void add(ParTaskImplInstance auto &task, const T &stage) {
      using InputT = typename T::InputT;
      using OutputT = typename T::OutputT;

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
  template <size_t I, ApplicableRefT RefT>
    inline void add(SeqTaskImplInstance auto &task, RefT stage) {
      using InternalT = typename RefT::type;

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
  template <size_t I, ApplicableRefT RefT>
    inline void add(ParTaskImplInstance auto &task, RefT stage) {
      using InternalT = typename RefT::type;

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
  // TODO(dk6): use ApplicableT instead StageT
  template <StageT S>
    typename S::TaskT apply(const S &stage, FunctionWrapper<typename S::InputT(void)> &&getter) {
      using TaskImplT = S::TaskImplT;
      using TaskT = S::TaskT;

      auto task = std::make_unique<TaskImplT>(std::move(getter));
      [&task, &stage]<size_t ...Is>(std::index_sequence<Is...>) {
        (detail::add<Is>(*task.get(), detail::to_wrapper_view_v(std::get<Is>(stage.stages))), ...);
      }(std::make_index_sequence<std::tuple_size_v<decltype(stage.stages)>>());

      return task;
    }

  template <StageT S>
    typename S::TaskT apply(const S &stage, typename S::InputT &&initial) {
      using TaskImplT = S::TaskImplT;
      using TaskT = S::TaskT;

      auto task = std::make_unique<TaskImplT>(std::move(initial));
      [&task, &stage]<size_t ...Is>(std::index_sequence<Is...>) {
        (detail::add<Is>(*task.get(), detail::to_wrapper_view_v(std::get<Is>(stage.stages))), ...);
      }(std::make_index_sequence<std::tuple_size_v<decltype(stage.stages)>>());

      return task;
    }
}


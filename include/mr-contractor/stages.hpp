#pragma once

#include "def.hpp"
#include "traits.hpp"
#include "task.hpp"

namespace mr::detail {
  template <typename T>
    struct to_wrapper {
      using type = T;
    };
  template <Callable T>
    struct to_wrapper<T> {
      using type = FunctionWrapper<output_t<T>(input_t<T>)>;
    };
  template <typename T> using to_wrapper_t = to_wrapper<T>::type;
  template <typename T> to_wrapper_t<T> to_wrapper_v(T &&stage);

  template <typename T>
    struct to_wrapper_view {
      using type = const T &;
    };
  template <Callable T>
    struct to_wrapper_view<T> {
      using type = FunctionView<output_t<T>(input_t<T>)>;
    };
  template <typename T> using to_wrapper_view_t = to_wrapper_view<T>::type;
  template <typename T> to_wrapper_view_t<T> to_wrapper_view_v(const T &v) { return v; }
}

namespace mr {
  template <typename ...> struct Sequence { static_assert(false, "ERROR: Some of the stages do not satisfy mr::StageT concept"); };
  template <typename ...> struct Parallel { static_assert(false, "ERROR: Some of the stages do not satisfy mr::StageT concept"); };

  template <typename T> constexpr bool is_sequence = false;
  template <typename ...Us> constexpr bool is_sequence<Sequence<Us...>> = true;
  template <typename T> concept SequenceT = is_sequence<T>;

  template <typename T> constexpr bool is_parallel = false;
  template <typename ...Us> constexpr bool is_parallel<Parallel<Us...>> = true;
  template <typename T> concept ParallelT = is_parallel<T>;

  template <typename T> concept StageT = Callable<T> || ParallelT<T> || SequenceT<T>;

  template <StageT ...StageTs> requires (sizeof...(StageTs) > 0)
    struct Parallel<StageTs...> {
    private:
    public:
      // for external use
      using InputT = mr::to_tuple_t<input_t<StageTs>...>;
      using OutputT = mr::to_tuple_t<output_t<StageTs>...>;
      using VariantT = mr::to_variant_t<input_t<StageTs>..., output_t<StageTs>...>;
      using TupleT = mr::to_tuple_t<mr::detail::to_wrapper_t<StageTs>...>;

      using TaskT = Task<OutputT>;
      using TaskImplT = detail::ParTaskImpl<sizeof...(StageTs), InputT, OutputT>;

      TupleT stages;
      constexpr Parallel(StageTs... s) : stages(std::forward_as_tuple(detail::to_wrapper_v(std::move(s))...)) {}
    };

  template <StageT ...StageTs>
    struct CallableTraits<Parallel<StageTs...>> {
      using InputT = Parallel<StageTs...>::InputT;
      using OutputT = Parallel<StageTs...>::OutputT;
    };

  template <typename ...StageTs>
    Parallel(StageTs ...stages) -> Parallel<StageTs...>;

  template <StageT ...StageTs> requires (sizeof...(StageTs) > 0)
    struct Sequence<StageTs...> {
    private:
      // for internal use
      static_assert(
          mr::erase_v(0, mp::vector{mp::meta<input_t<StageTs>>...}) ==
          mr::erase_v(sizeof...(StageTs) - 1, mp::vector{mp::meta<output_t<StageTs>>...})
        , "Invalid transform chain (type mismatch)");

    public:
      // for external use
      using InputT = at_t<0, input_t<StageTs>...>;
      using OutputT = at_t<sizeof...(StageTs)-1, output_t<StageTs>...>;
      using VariantT = mr::to_variant_t<input_t<StageTs>..., output_t<StageTs>...>;
      using TupleT = mr::to_tuple_t<mr::detail::to_wrapper_t<StageTs>...>;
      using TaskT = Task<OutputT>;
      using TaskImplT = detail::SeqTaskImpl<sizeof...(StageTs), VariantT, InputT, OutputT>;

      TupleT stages;
      constexpr Sequence(StageTs... s) : stages(detail::to_wrapper_v(std::move(s))...) {}
    };

  template <StageT ...StageTs>
    struct CallableTraits<Sequence<StageTs...>> {
      using InputT = Sequence<StageTs...>::InputT;
      using OutputT = Sequence<StageTs...>::OutputT;
    };

  template <typename ...StageTs>
    Sequence(StageTs... stages) -> Sequence<StageTs...>;

  template <typename T> concept ApplicableT = ParallelT<T> || SequenceT<T>;

  template <StageT S> typename S::TaskT apply(const S &stage, FunctionWrapper<typename S::InputT(void)> &&getter);
  template <StageT S> typename S::TaskT apply(const S &stage, typename S::InputT &&initial);

  template <typename ResultT, typename ...Args> ApplicableT decltype(auto) make_task_prototype() {
    static_assert(false, "You need to overload default implementation, \n"
                         "so that the function returns valid Sequence/Parallel. \n"
                         "Note that after Sequence/Parallel destruction, \n"
                         " all Tasks created from it become invalid."
                         );
  };
}

namespace mr::detail {
  template <ApplicableT T>
    struct to_wrapper<T> {
      using type = FunctionWrapper<typename T::OutputT(typename T::InputT)>;
    };

  template <typename T>
    to_wrapper_t<T> to_wrapper_v(T&& stage) {
      if constexpr (Callable<T>) {
        // Wrap raw callables directly
        return FunctionWrapper<output_t<T>(input_t<T>)>(std::forward<T>(stage));
      }
      else if constexpr (ApplicableT<T>) {
        // Recursively convert nested Parallel/Sequence and wrap as function
        using InputT = typename T::InputT;
        using OutputT = typename T::OutputT;

        return FunctionWrapper<OutputT(InputT)>(
          [inner_stage=std::forward<T>(stage)](InputT input) mutable {
            auto task = mr::apply(inner_stage, std::move(input));
            task->execute();
            return task->result();
          }
        );
      }
      else {
        static_assert(false, "Unsupported stage type");
      }
    }
}

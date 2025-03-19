#pragma once

#include <vtll/vtll.hpp>

#include "mr-contractor/def.hpp"
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
      using InputsListT = vtll::type_list<input_t<StageTs>...>;
      using OutputsListT = vtll::type_list<output_t<StageTs>...>;
      using InputOutputListT = vtll::type_list<input_t<StageTs>..., output_t<StageTs>...>;
      using UniqueInputOutputListT = vtll::remove_duplicates<InputOutputListT>;

    public:
      // for external use
      using InputT = vtll::to_tuple<InputsListT>;
      using OutputT = vtll::to_tuple<OutputsListT>;
      using VariantT = vtll::to_variant<UniqueInputOutputListT>;
      using TupleT = vtll::to_tuple<vtll::transform<vtll::type_list<StageTs...>, mr::detail::to_wrapper_t>>;
      using NestedTasksTupleT = std::tuple<>;
      using TaskT = Task<OutputT>;
      using TaskImplT = detail::ParTaskImpl<sizeof...(StageTs), InputT, OutputT, NestedTasksTupleT>;

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
      using InputsListT = vtll::type_list<input_t<StageTs>...>;
      using OutputsListT = vtll::type_list<output_t<StageTs>...>;
      using InputOutputListT = vtll::type_list<input_t<StageTs>..., output_t<StageTs>...>;

      static_assert(vtll::is_same_list<
          vtll::erase_Nth<InputsListT, 0>,
          vtll::erase_Nth<OutputsListT, vtll::size<OutputsListT>::value - 1>
        >::value, "Invalid transform chain (type mismatch)");

      using UniqueInputOutputListT = vtll::remove_duplicates<InputOutputListT>;

    public:
      // for external use
      using InputT = vtll::front<InputsListT>;
      using OutputT = vtll::back<OutputsListT>;
      using VariantT = vtll::to_variant<UniqueInputOutputListT>;
      using NestedTasksTupleT = std::tuple<>;
      using TupleT = vtll::to_tuple<vtll::transform<vtll::type_list<StageTs...>, mr::detail::to_wrapper_t>>;
      using TaskT = Task<OutputT>;
      using TaskImplT = detail::SeqTaskImpl<vtll::size<OutputsListT>::value, VariantT, OutputT, NestedTasksTupleT>;

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

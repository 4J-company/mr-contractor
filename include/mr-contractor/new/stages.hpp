#pragma once

#include <vtll/vtll.hpp>

#include "mr-contractor/def.hpp"
#include "traits.hpp"
#include "task.hpp"

namespace mr {
  template <typename ...> struct Sequence { static_assert(false, "ERROR: Some of the stages do not satisfy mr::StageT concept"); };
  template <typename ...> struct Parallel { static_assert(false, "ERROR: Some of the stages do not satisfy mr::StageT concept"); };

  template <typename T> constexpr bool is_sequence = false;
  template <typename ...Us> constexpr bool is_sequence<Sequence<Us...>> = true;
  template <typename T> concept SequenceT = is_sequence<T>;

  template <typename T> constexpr bool is_parallel = false;
  template <typename ...Us> constexpr bool is_parallel<Parallel<Us...>> = true;
  template <typename T> concept ParallelT = is_parallel<T>;

  template <typename T> concept StageT = FunctionT<T> || ParallelT<T> || SequenceT<T>;

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
      using TupleT = std::tuple<StageTs...>;
      using TaskT = Task<OutputT>;
      using TaskImplT = detail::ParTaskImpl<sizeof...(StageTs), InputT, OutputT>;

      TupleT stages;
      constexpr Parallel(StageTs... s) : stages(std::forward_as_tuple(s...)) {}
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
      using TupleT = std::tuple<StageTs...>;
      using TaskT = Task<OutputT>;
      using TaskImplT = detail::SeqTaskImpl<VariantT, OutputT>;

      TupleT stages;
      constexpr Sequence(StageTs... s) : stages(std::forward_as_tuple(s...)) {}
    };

  template <typename ...StageTs>
    Sequence(StageTs... stages) -> Sequence<StageTs...>;
}

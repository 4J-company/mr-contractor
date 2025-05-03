#pragma once

#include "def.hpp"
#include "traits.hpp"
#include "task.hpp"

namespace mr::detail {
  template <typename T>
    struct to_wrapper : std::type_identity<T> {};

  template <Callable T>
    struct to_wrapper<T> : std::type_identity<FunctionWrapper<func_t<T>>> {};

  template <typename T> using to_wrapper_t = to_wrapper<T>::type;
  template <typename T> to_wrapper_t<T> to_wrapper_v(T &&stage);

  template <typename T>
    struct to_wrapper_view : std::type_identity<const T&> {};

  template <Callable T>
    struct to_wrapper_view<T> : std::type_identity <FunctionView<func_t<T>>> {};

  template <typename T> using to_wrapper_view_t = to_wrapper_view<T>::type;
  template <typename T> to_wrapper_view_t<T> to_wrapper_view_v(const T &v) { return v; }
}

namespace mr {
  template <typename ...>
    struct Sequence { static_assert(false, "ERROR: Some of the stages do not satisfy mr::Stage concept"); };

    template <typename ...>
    struct Parallel { static_assert(false, "ERROR: Some of the stages do not satisfy mr::Stage concept"); };

  template <typename T>
    concept Applicable = InstanceOf<T, Parallel> || InstanceOf<T, Sequence>;

  template <typename T>
   concept ApplicableRef = InstanceOf<T, std::reference_wrapper>;

  template <typename T>
    concept Stage = Callable<T> || Applicable<T> || ApplicableRef<T>;

  namespace detail {
    template <Applicable T>
      struct to_wrapper<T> {
        using type = FunctionWrapper<typename T::OutputT(typename T::InputT)>;
      };

    template <Applicable T>
      struct to_wrapper<std::reference_wrapper<T>> {
        using type = typename to_wrapper<T>::type;
      };
  }

  template <Stage ...StageTs> requires (sizeof...(StageTs) > 0)
    struct Parallel<StageTs...> {
    public:
      static constexpr size_t sub_stages_count = sizeof...(StageTs);

      using InputT = mr::to_tuple_t<input_t<StageTs>...>;
      using OutputT = mr::to_tuple_t<output_t<StageTs>...>;
      using VariantT = mr::to_variant_t<input_t<StageTs>..., output_t<StageTs>...>;
      using TupleT = mr::to_tuple_t<mr::detail::to_wrapper_t<StageTs>...>;

      using TaskT = Task<OutputT>;
      using TaskImplT = detail::ParTaskImpl<std::integral_constant<size_t, sub_stages_count>, InputT, OutputT>;

      constexpr Parallel(StageTs... s) : stages(std::forward_as_tuple(detail::to_wrapper_v(std::move(s))...)) {}

      TupleT stages;
    };

  template <Stage... StageTs>
    struct CallableTraits<Parallel<StageTs...>> {
      using InputT = Parallel<StageTs...>::InputT;
      using OutputT = Parallel<StageTs...>::OutputT;
    };

  template <typename... StageTs>
    Parallel(StageTs... stages) -> Parallel<StageTs...>;

  template <Stage... StageTs> requires (sizeof...(StageTs) > 0)
    struct Sequence<StageTs...> {
    public:
      static constexpr size_t sub_stages_count = sizeof...(StageTs);

      using InputT = at_t<0, input_t<StageTs>...>;
      using OutputT = at_t<sub_stages_count - 1, output_t<StageTs>...>;
      using VariantT = mr::to_variant_t<input_t<StageTs>..., output_t<StageTs>...>;
      using TupleT = mr::to_tuple_t<mr::detail::to_wrapper_t<StageTs>...>;
      using TaskT = Task<OutputT>;
      using TaskImplT = detail::SeqTaskImpl<std::integral_constant<size_t, sub_stages_count>, VariantT, InputT, OutputT>;

      constexpr Sequence(StageTs... s) : stages(detail::to_wrapper_v(std::move(s))...) {}

      TupleT stages;

    private:
      static_assert(
          mr::erase_v(0, mp::vector{mp::meta<input_t<StageTs>>...}) ==
          mr::erase_v(sub_stages_count - 1, mp::vector{mp::meta<output_t<StageTs>>...})
        , "Invalid transform chain (type mismatch)");
    };

  template <Stage... StageTs>
    struct CallableTraits<Sequence<StageTs...>> {
      using InputT = Sequence<StageTs...>::InputT;
      using OutputT = Sequence<StageTs...>::OutputT;
    };

  template <typename... StageTs>
    Sequence(StageTs... stages) -> Sequence<StageTs...>;

  template <Stage StageT>
    struct CallableTraits<std::reference_wrapper<StageT>> {
      using InputT = input_t<StageT>;
      using OutputT = output_t<StageT>;
    };

  template <Stage StageT> typename StageT::TaskT apply(const StageT &stage, FunctionWrapper<typename StageT::InputT(void)> &&getter);
  template <Stage StageT> typename StageT::TaskT apply(const StageT &stage, typename StageT::InputT initial);
}

namespace mr::detail {
  template <typename T>
    to_wrapper_t<T> to_wrapper_v(T&& stage) {
      if constexpr (Callable<T>) {
        // Wrap raw callables directly
        return FunctionWrapper<func_t<T>>(std::forward<T>(stage));
      }
      else if constexpr (Applicable<T>) {
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
      } else if constexpr (ApplicableRef<T>) {
        using RealStage = typename T::type;

        using InputT = input_t<RealStage>;
        using OutputT = output_t<RealStage>;

        return FunctionWrapper<OutputT(InputT)>(
          [stage_ref = stage](InputT input) mutable {
            auto task = mr::apply(stage_ref.get(), std::move(input));
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

#pragma once

#include "mr-contractor/def.hpp"

namespace mr::details {
  template <typename L> using to_std_function =
    std::invoke_result_t<decltype([](const L &l) {
      return std::function(l);
    }), L>;
}

namespace mr {
  template <typename T> struct FuncTraits {
    using InputT = typename T::InputT;
    using OutputT = typename T::OutputT;
  };
  template <typename O, typename I> struct FuncTraits<O(I)> {
    using InputT = I;
    using OutputT = O;
  };
  template <typename O, typename I> struct FuncTraits<std::function<O(I)>> {
    using InputT = I;
    using OutputT = O;
  };
  template <typename T> struct FuncTraits<const T> {
    using InputT = FuncTraits<T>::InputT;
    using OutputT = FuncTraits<T>::OutputT;
  };
  template <typename T> struct FuncTraits<T &> {
    using InputT = FuncTraits<T>::InputT;
    using OutputT = FuncTraits<T>::OutputT;
  };
  template <typename T> struct FuncTraits<const T &> {
    using InputT = FuncTraits<T>::InputT;
    using OutputT = FuncTraits<T>::OutputT;
  };
  template <typename T> FuncTraits(T t) -> FuncTraits<T>;

  template <typename T>
    using input_t = FuncTraits<T>::InputT;
  template <typename T>
    using output_t = FuncTraits<T>::OutputT;

  template <typename T> constexpr bool is_function = is_function<details::to_std_function<T>>;
  template <typename U> constexpr bool is_function<std::function<U>> = std::is_function_v<U>;
  template <typename T> concept FunctionT = requires {is_function<T>;};
}

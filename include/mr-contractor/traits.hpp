#pragma once

#include <function2/function2.hpp>

namespace mr {
  // Primary template declaration
  template<typename FuncT>
    struct CallableTraits;

  // Specialization for function pointers
  template<typename R, typename A>
    struct CallableTraits<R(*)(A)> {
      using InputT = A;
      using OutputT = R;
    };

  // Specializations for fu2
  template<typename Signature>
    struct CallableTraits<fu2::function<Signature>> {
      private:
        template<typename T>
          struct DecomposeFunction;

        template<typename R, typename A>
          struct DecomposeFunction<R(A)> {
            using InputT = A;
            using OutputT = R;
          };

      public:
        using InputT = typename DecomposeFunction<Signature>::InputT;
        using OutputT = typename DecomposeFunction<Signature>::OutputT;
    };

  template<typename Signature>
    struct CallableTraits<fu2::function_view<Signature>> {
      private:
        template<typename T>
          struct DecomposeFunction;

        template<typename R, typename A>
          struct DecomposeFunction<R(A)> {
            using InputT = A;
            using OutputT = R;
          };

      public:
        using InputT = typename DecomposeFunction<Signature>::InputT;
        using OutputT = typename DecomposeFunction<Signature>::OutputT;
    };

  template<typename Signature>
    struct CallableTraits<fu2::unique_function<Signature>> {
      private:
        template<typename T>
          struct DecomposeFunction;

        template<typename R, typename A>
          struct DecomposeFunction<R(A)> {
            using InputT = A;
            using OutputT = R;
          };

      public:
        using InputT = typename DecomposeFunction<Signature>::InputT;
        using OutputT = typename DecomposeFunction<Signature>::OutputT;
    };

  // Helper to decompose member function pointer types (operator() for functors/lambdas)
  template<typename T>
    struct CallableTraitsImpl;

  // Non-const versions
  template<typename R, typename C, typename A>
    struct CallableTraitsImpl<R (C::*)(A)> {
      using InputT = A;
      using OutputT = R;
    };

  template<typename R, typename C, typename A>
    struct CallableTraitsImpl<R (C::*)(A) const> {
      using InputT = A;
      using OutputT = R;
    };

  template<typename R, typename C, typename A>
    struct CallableTraitsImpl<R (C::*)(A) &> {
      using InputT = A;
      using OutputT = R;
    };

  template<typename R, typename C, typename A>
    struct CallableTraitsImpl<R (C::*)(A) const &> {
      using InputT = A;
      using OutputT = R;
    };

  template<typename R, typename C, typename A>
    struct CallableTraitsImpl<R (C::*)(A) &&> {
      using InputT = A;
      using OutputT = R;
    };

  template<typename R, typename C, typename A>
    struct CallableTraitsImpl<R (C::*)(A) const &&> {
      using InputT = A;
      using OutputT = R;
    };

  // Primary template for functors, lambdas, and other callable objects
  template<typename FuncT>
    struct CallableTraits {
      private:
        using BareT = std::remove_cv_t<std::remove_reference_t<FuncT>>;
        using CallOperatorType = decltype(&BareT::operator());
        using Impl = CallableTraitsImpl<CallOperatorType>;

      public:
        using InputT = typename Impl::InputT;
        using OutputT = typename Impl::OutputT;
    };

  // Concept to check if T is a valid callable with one argument
  template<typename T>
    concept Callable = requires {
      // Ensure CallableTraits<T> provides InputT and OutputT
      typename CallableTraits<std::remove_cvref_t<T>>::InputT;
      typename CallableTraits<std::remove_cvref_t<T>>::OutputT;
      // Ensure T can be called with InputT and returns OutputT
      requires std::is_invocable_r_v<
        typename CallableTraits<std::remove_cvref_t<T>>::OutputT,
        T,
        typename CallableTraits<std::remove_cvref_t<T>>::InputT
      >;
    };

  template <typename F> using input_t = CallableTraits<F>::InputT;
  template <typename F> using output_t = CallableTraits<F>::OutputT;
}

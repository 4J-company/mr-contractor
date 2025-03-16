#pragma once

#include <function2/function2.hpp>

namespace mr {

  namespace details {

    template <typename I, typename O>
      struct InputTOutputT {
        using InputT = I;
        using OutputT = O;
      };

  }

  // Primary template declaration
  template<typename FuncT>
    struct CallableTraits;

  // Specialization for plain function types
  template<typename R, typename A>
    struct CallableTraits<R(A)> : details::InputTOutputT<A, R> {};

  // Specialization for function pointers
  template<typename R, typename A>
    struct CallableTraits<R(*)(A)> : details::InputTOutputT<A, R> {};

  // Specializations for fu2
  template<typename R, typename A>
    struct CallableTraits<fu2::function<R(A)>> : details::InputTOutputT<A, R> {};

  template<typename R, typename A>
    struct CallableTraits<fu2::function_view<R(A)>> : details::InputTOutputT<A, R> {};

  template<typename R, typename A>
    struct CallableTraits<fu2::unique_function<R(A)>> : details::InputTOutputT<A, R> {};

  // Helper to decompose member function pointer types (operator() for functors/lambdas)
  template<typename T>
    struct CallableMemberTraits;

  // non-volatile specializations
  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R (C::*)(A)> : details::InputTOutputT<A, R> {};

  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R (C::*)(A) const> : details::InputTOutputT<A, R> {};
  
  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R (C::*)(A) &> : details::InputTOutputT<A, R> {};
  
  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R (C::*)(A) const &> : details::InputTOutputT<A, R> {};
  
  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R (C::*)(A) &&> : details::InputTOutputT<A, R> {};
  
  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R (C::*)(A) const &&> : details::InputTOutputT<A, R> {};

  // volatile specializations
  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R(C::*)(A) volatile> : details::InputTOutputT<A, R> {};

  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R(C::*)(A) const volatile> : details::InputTOutputT<A, R> {};

  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R(C::*)(A) volatile &> : details::InputTOutputT<A, R> {};

  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R(C::*)(A) const volatile &> : details::InputTOutputT<A, R> {};

  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R(C::*)(A) volatile &&> : details::InputTOutputT<A, R> {};

  template<typename R, typename C, typename A>
    struct CallableMemberTraits<R(C::*)(A) const volatile &&> : details::InputTOutputT<A, R> {};

  // Primary template for functors, lambdas, and other callable objects
  template<typename FuncT>
    struct CallableTraits {
      private:
        using BareT = std::remove_cvref_t<FuncT>;
        using CallOperatorType = decltype(&BareT::operator());
        using Impl = CallableMemberTraits<std::remove_cvref_t<CallOperatorType>>;

      public:
        using InputT = typename Impl::InputT;
        using OutputT = typename Impl::OutputT;
    };

  template <typename F> using input_t = CallableTraits<std::remove_cvref_t<F>>::InputT;
  template <typename F> using output_t = CallableTraits<std::remove_cvref_t<F>>::OutputT;

  // Concept to check if T is a valid callable with one argument
  template<typename T>
    concept Callable = requires {
      // Ensure CallableTraits<T> provides InputT and OutputT
      typename input_t<T>;
      typename output_t<T>;
      // Ensure T can be called with InputT and returns OutputT
      requires std::is_invocable_r_v<output_t<T>, T, input_t<T>>;
    };
}

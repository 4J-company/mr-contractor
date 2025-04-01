#pragma once

#include <tuple>
#include <functional>
#include <type_traits>
#include <variant>
#include <ranges>

#include <mp/mp>
#include <work_contract/work_contract.h>

#include "traits.hpp"

namespace mr {
  template <typename Signature> using FunctionWrapper = fu2::unique_function<Signature>;
  template <typename Signature> using FunctionView = fu2::function_view<Signature>;

  using Contract = bcpp::work_contract;

  // meta-functions
  template <typename ...Ts>
    using to_tuple_t = std::tuple<Ts...>;

  template <typename ...Ts>
    constexpr mp::vector<mp::info> unique_v(std::ranges::range auto &&ts) {
      mp::vector<mp::info> res;

      for (auto t : ts) {
        bool contains = false;
        for (auto r : res) {
          if (r == t) {
            contains = true;
            break;
          }
        }
        if (!contains) {
          res.push_back(t);
        }
      }

      return res;
    }

  template <typename ...Ts>
    using to_variant_t = mp::apply_t<std::variant, unique_v(std::array{mp::meta<Ts>...})>;

  template <typename ...Ts>
    constexpr mp::vector<mp::info> erase_v(size_t ind, std::ranges::range auto &&ts) {
      mp::vector<mp::info> res;
      for (int i = 0; i < ts.size(); i++) {
        if (i != ind) {
          res.push_back(ts[i]);
        }
      }
      return res;
    }

  template<size_t N, class... Ts>
    using at_t = mp::type_of<std::array{mp::meta<Ts>...}[N]>;
}

#pragma once

#include <tuple>
#include <functional>
#include <type_traits>

#include <work_contract/work_contract.h>

#include "traits.hpp"

namespace mr {
  template <typename Signature> using FunctionWrapper = fu2::unique_function<Signature>;
  template <typename Signature> using FunctionView = fu2::function_view<Signature>;

  using Contract = bcpp::work_contract;
}

#include <gtest/gtest.h>

#include <mr-contractor/contractor.hpp>

using namespace std::literals;
using namespace mr;

// Helper functions for testing
auto add_one = [](int x) { return x + 1; };
auto multiply_by_two = [](int x) { return x * 2; };
auto to_string = [](int x) { return std::to_string(x); };

TEST(SequenceTest, SingleStage) {
  auto seq = Sequence {add_one};
  auto task = mr::apply(seq, 5);
  task->schedule().wait();
  EXPECT_EQ(task->result(), 6);
}

TEST(SequenceTest, MultiStage) {
  auto seq = Sequence {add_one, multiply_by_two};
  auto task = mr::apply(seq, 5);
  task->schedule().wait();
  EXPECT_EQ(task->result(), 12);
}


TEST(ParallelTest, SingleStage) {
  auto par = Parallel {add_one};
  auto task = mr::apply(par, {1});
  task->schedule().wait();
  EXPECT_EQ(task->result(), std::tuple(2));
}

TEST(ParallelTest, MultipleStage) {
  auto par = Parallel {add_one, multiply_by_two};
  auto task = mr::apply(par, {1, 2});
  task->schedule().wait();
  EXPECT_EQ(task->result(), std::tuple(2, 4));
}


TEST(CompositeTest, SequenceOfParallels) {
  auto com = Sequence {
    Parallel {add_one, multiply_by_two},
    Parallel {to_string, to_string}
  };

  auto task = mr::apply(com, {1, 2});
  task->schedule().wait();
  EXPECT_EQ(task->result(), std::tuple("2"s, "4"s));
}

TEST(CompositeTest, SequenceOfParallelsOfSequences) {
  auto prototype = mr::Sequence {
    [](int x) -> std::tuple<int, int> { return {x, 2*x}; },
      mr::Parallel {
        mr::Sequence { // Nested Sequence inside Parallel
          [](int a) -> float { return a * 2; },
          [](float b) -> double { return b / 3.0; }
        },
        mr::Sequence { // Another nested Sequence
          [](int b) -> int { return b + 5; },
          [](int c) -> float { return c * 1.5f; }
        }
      },
      [](std::tuple<double, float> y) -> int { return std::get<0>(y) + std::get<1>(y); }
  };

  auto task = mr::apply(prototype, 47);
  task->execute();
  EXPECT_EQ(task->result(), 179);
}

TEST(CompositeTest, Decomposition) {
  auto task = mr::Sequence {
    [](int x) -> std::tuple<int, int> { return {x, x * 2}; },
    mr::Parallel {
      mr::Sequence {  // Nested sequential steps
        [](int a) -> int         { return a + 1; },
        [](int b) -> std::string { return std::to_string(b); }
      },
      [](int c) -> float { return c * 0.5f; }
    },
    [](std::tuple<std::string, float> inputs) {
      auto&& [str, flt] = inputs;
      return str + " @ " + std::to_string(flt);
    }
  };

  auto result = apply(task, 5)->execute().result();
  EXPECT_EQ(result, "6 @ 5.000000"s);
}

TEST(ReferenceTest, Sequence) {
  auto seq1 = mr::Sequence {
    [](int a) -> int { return a + 47; }
  };
  auto seq2 = mr::Sequence {
    [](int a) -> int { return a + 102; }
  };
  auto func = [b = std::make_unique<int>(10)](int a) { return a + 30 + *b; };

  auto task = mr::Sequence {
    std::ref(seq1),
    std::move(seq2),
    std::ref(func)
  };

  // test using func
  auto lambda_res = func(7);
  EXPECT_EQ(lambda_res, 47);

  // test using subsequence
  auto subres = apply(seq1, 0)->execute().result();
  EXPECT_EQ(subres, 0 + 47);

  auto result = apply(task, 0)->execute().result();
  EXPECT_EQ(result, 0 + 47 + 102 + 30 + 10);
}

TEST(ReferenceTest, Parallel) {
  auto internal_seq = mr::Sequence {
    [](int a) -> int { return a + 47; }
  };
  auto par = mr::Parallel {
    [](int a) -> int { return a + 102; },
    std::ref(internal_seq)
  };
  auto external_seq = mr::Sequence {
    std::ref(par),
    [](std::tuple<int, int> t) -> int {
      auto [a, b] = t;
      return a + b;
    }
  };

  auto int_res = mr::apply(internal_seq, 0)->execute().result();
  EXPECT_EQ(int_res, 47);

  auto par_res = mr::apply(par, std::make_tuple(0, 0))->execute().result();
  EXPECT_EQ(par_res, std::make_tuple(0 + 102, 0 + 47));

  auto res = mr::apply(external_seq, std::make_tuple(0, 0))->execute().result();
  EXPECT_EQ(res, 0 + 102 + 0 + 47);
}
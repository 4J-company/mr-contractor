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

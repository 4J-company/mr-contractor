#include <gtest/gtest.h>

#include <mr-contractor/contractor.hpp>

#include <mr-contractor/new/apply.hpp>
#include <mr-contractor/new/stages.hpp>
#include <mr-contractor/new/task.hpp>
#include <mr-contractor/new/traits.hpp>

using namespace mr;

// Helper functions for testing
auto add_one = std::function([](int x) { return x + 1; });
auto multiply_by_two = std::function([](int x) { return x * 2; });
auto to_string = std::function([](int x) { return std::to_string(x); });

TEST(PipePrototypeTest, SingleStagePipeline) {
    auto prototype = PipePrototype{add_one};
    auto pipe = prototype.on(5);
    pipe->schedule().wait();
    EXPECT_EQ(pipe->result(), 6);
}

TEST(PipePrototypeTest, MultiStagePipeline) {
    auto prototype = PipePrototype{add_one, multiply_by_two};
    auto pipe = prototype.on(5);
    pipe->schedule().wait();
    EXPECT_EQ(pipe->result(), 12);
}

TEST(TaskTest, SingleStageSequence) {
  auto seq = Sequence {add_one};
  auto task = apply(seq, 5);
  task->schedule().wait();
  EXPECT_EQ(task->result(), 6);
}

TEST(TaskTest, MultiStageSequence) {
  auto seq = Sequence {add_one, multiply_by_two};
  auto task = apply(seq, 5);
  task->schedule().wait();
  EXPECT_EQ(task->result(), 12);
}

TEST(TaskTest, Parallel) {
  auto par = Parallel {add_one, multiply_by_two};
  auto task = apply(par, {1, 2});
  task->schedule().wait();
  EXPECT_EQ(task->result(), std::tuple(2, 4));
}

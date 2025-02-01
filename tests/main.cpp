#include <gtest/gtest.h>

#include <mr-contractor/pipe.hpp>

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

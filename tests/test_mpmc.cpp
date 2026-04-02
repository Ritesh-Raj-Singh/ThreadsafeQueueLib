#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "tsfqueue.hpp"

// Basic sanity
TEST(MPMCQueue, BasicPushPop_Unbounded) {
  tsfqueue::BlockingMPMCUnbounded<int> q;

  EXPECT_TRUE(q.empty());

  q.push(1);
  q.push(2);

  int x;
  EXPECT_TRUE(q.try_pop(x));
  EXPECT_EQ(x, 1);

  EXPECT_TRUE(q.try_pop(x));
  EXPECT_EQ(x, 2);

  EXPECT_TRUE(q.empty());
}
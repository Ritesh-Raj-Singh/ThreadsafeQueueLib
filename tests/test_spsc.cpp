#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "tsfqueue.hpp"

// Basic sanity
TEST(SPSCQueue, BasicPushPop_Bounded) {
  tsfqueue::SPSCBounded<int, 4> q;

  EXPECT_TRUE(q.empty());

  EXPECT_TRUE(q.try_push(1));
  EXPECT_TRUE(q.try_push(2));

  int x;
  EXPECT_TRUE(q.try_pop(x));
  EXPECT_EQ(x, 1);

  EXPECT_TRUE(q.try_pop(x));
  EXPECT_EQ(x, 2);

  EXPECT_TRUE(q.empty());
}

// Basic sanity
TEST(SPSCQueue, BasicPushPop_Unbounded) {
  tsfqueue::SPSCUnbounded<int> q;

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
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "tsfqueue.hpp"

// Basic sanity
TEST(MPSCQueue, BasicPushPop_Unbounded) {
  tsfqueue::MPSCUnbounded<int> q;

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

// Stress test for unbounded MPSC
TEST(MPSCQueue, DataRaceStressTest_Unbounded) {
  tsfqueue::MPSCUnbounded<int> q;
  const int num_producers = 8;
  const int ops_per_producer = 10000;
  std::atomic<int> total_popped{0};

  std::vector<std::thread> producers;

  for (int i = 0; i < num_producers; ++i) {
    producers.emplace_back([&q, ops_per_producer]() {
      for (int j = 0; j < ops_per_producer; ++j) {
        q.push(j);
      }
    });
  }

  std::thread consumer([&q, num_producers, ops_per_producer, &total_popped]() {
    int expected_total = num_producers * ops_per_producer;
    while (total_popped.load() < expected_total) {
      int val;
      if (q.try_pop(val)) {
        total_popped++;
      } else {
        std::this_thread::yield();
      }
    }
  });

  for (auto &t : producers) {
    t.join();
  }
  consumer.join();

  EXPECT_EQ(total_popped.load(), num_producers * ops_per_producer);
  EXPECT_TRUE(q.empty());
}

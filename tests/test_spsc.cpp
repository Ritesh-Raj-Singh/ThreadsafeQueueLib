#include <atomic>
#include <chrono>
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

TEST(SPSCQueue, BasicPushPop_Fast) {
  tsfqueue::FastSPSCUnbounded<int> q;

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

// Stress test for bounded SPSC
TEST(SPSCQueue, DataRaceStressTest_Bounded) {
  tsfqueue::SPSCBounded<int, 1024> q;
  const int ops = 100000;
  std::atomic<int> total_popped{0};

  std::thread producer([&q, ops]() {
    for (int j = 0; j < ops; ++j) {
      while (!q.try_push(j)) {
        std::this_thread::yield();
      }
    }
  });

  std::thread consumer([&q, ops, &total_popped]() {
    for (int j = 0; j < ops; ++j) {
      int val;
      auto start = std::chrono::high_resolution_clock::now();
      bool popped = false;
      while (!popped) {
        if (q.try_pop(val)) {
          total_popped++;
          popped = true;
        } else {
          auto now = std::chrono::high_resolution_clock::now();
          if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000) {
            break; // timeout
          }
          std::this_thread::yield();
        }
      }
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(total_popped.load(), ops);
  EXPECT_TRUE(q.empty());
}

// Stress test for unbounded SPSC
TEST(SPSCQueue, DataRaceStressTest_Unbounded) {
  tsfqueue::SPSCUnbounded<int> q;
  const int ops = 100000;
  std::atomic<int> total_popped{0};

  std::thread producer([&q, ops]() {
    for (int j = 0; j < ops; ++j) {
      q.push(j);
    }
  });

  std::thread consumer([&q, ops, &total_popped]() {
    for (int j = 0; j < ops; ++j) {
      int val;
      auto start = std::chrono::high_resolution_clock::now();
      bool popped = false;
      while (!popped) {
        if (q.try_pop(val)) {
          total_popped++;
          popped = true;
        } else {
          auto now = std::chrono::high_resolution_clock::now();
          if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000) {
            break; // timeout
          }
          std::this_thread::yield();
        }
      }
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(total_popped.load(), ops);
  EXPECT_TRUE(q.empty());
}

// Emplace back test
struct MockObject {
  static int copies;
  static int moves;
  int x;
  MockObject() : x(0) {}
  MockObject(int val) : x(val) {}
  MockObject(const MockObject &other) { copies++; x = other.x; }
  MockObject(MockObject &&other) noexcept { moves++; x = other.x; }
  MockObject &operator=(const MockObject &other) {
    x = other.x;
    copies++;
    return *this;
  }
  MockObject &operator=(MockObject &&other) noexcept {
    x = other.x;
    moves++;
    return *this;
  }
};

int MockObject::copies = 0;
int MockObject::moves = 0;

TEST(SPSCQueue, EmplaceBackEfficiency_Bounded) {
  tsfqueue::SPSCBounded<MockObject, 10> q;
  MockObject::copies = 0;
  MockObject::moves = 0;
  
  q.emplace_back(42);
  EXPECT_EQ(MockObject::copies, 0);
  
  MockObject out(0);
  EXPECT_TRUE(q.try_pop(out));
  EXPECT_EQ(out.x, 42);
}

TEST(SPSCQueue, EmplaceBackEfficiency_Unbounded) {
  tsfqueue::SPSCUnbounded<MockObject> q;
  MockObject::copies = 0;
  MockObject::moves = 0;
  
  q.emplace_back(42);
  EXPECT_EQ(MockObject::copies, 0);
  
  MockObject out(0);
  EXPECT_TRUE(q.try_pop(out));
  EXPECT_EQ(out.x, 42);
}

struct LockedType {
  LockedType() = default;
  LockedType(const LockedType &) = delete;
  LockedType(LockedType &&) = delete;
  LockedType &operator=(const LockedType &) = delete;
  LockedType &operator=(LockedType &&) = delete;
};

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(const MoveOnly &) = delete;
  MoveOnly(MoveOnly &&) = default;
  MoveOnly &operator=(const MoveOnly &) = delete;
  MoveOnly &operator=(MoveOnly &&) = default;
};

TEST(SPSCQueue, StaticRequirementsValidation) {
  bool is_pushable =
      std::is_copy_constructible_v<int> || std::is_move_constructible_v<int>;
  EXPECT_TRUE(is_pushable);

  bool move_only_assignable = std::is_copy_assignable_v<MoveOnly> ||
                              std::is_move_assignable_v<MoveOnly>;
  EXPECT_TRUE(move_only_assignable);

  bool locked_type_assignable = std::is_copy_assignable_v<LockedType> ||
                                std::is_move_assignable_v<LockedType>;
  EXPECT_FALSE(locked_type_assignable);
}
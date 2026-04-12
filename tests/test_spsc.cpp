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

TEST(SPSCQueue, WaitAndPopBlocksUntilPush) {
  tsfqueue::SPSCUnbounded<int> q;
  EXPECT_TRUE(q.empty());

  constexpr int push_time_ms = 200;
  constexpr int error_ms = 50;

  std::thread producer([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(push_time_ms));
    q.push(12);
  });

  int x = 0;
  auto start = std::chrono::high_resolution_clock::now();
  q.wait_and_pop(x);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  EXPECT_NEAR(static_cast<int>(duration), push_time_ms, error_ms);
  EXPECT_EQ(x, 12);
  EXPECT_TRUE(q.empty());

  producer.join();
}

TEST(SPSCQueue, DataRaceStressTest) {
  tsfqueue::SPSCUnbounded<int> q;
  constexpr int ops = 100000;

  std::atomic<int> total_popped{0};
  std::atomic<long long> sum{0};

  std::thread producer([&]() {
    for (int i = 0; i < ops; ++i) {
      q.push(i);
    }
  });

  std::thread consumer([&]() {
    for (int i = 0; i < ops; ++i) {
      int val = -1;
      q.wait_and_pop(val);
      total_popped.fetch_add(1, std::memory_order_relaxed);
      sum.fetch_add(val, std::memory_order_relaxed);
    }
  });

  producer.join();
  consumer.join();

  const long long expected_sum = (static_cast<long long>(ops) * (ops - 1)) / 2;
  EXPECT_EQ(total_popped.load(), ops);
  EXPECT_EQ(sum.load(), expected_sum);
  EXPECT_TRUE(q.empty());
}

struct MockObject {
  static int copies;
  static int moves;
  int x;

  MockObject() : x(0) {}
  explicit MockObject(int val) : x(val) {}
  MockObject(const MockObject &) { copies++; }
  MockObject(MockObject &&) noexcept { moves++; }

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

TEST(SPSCQueue, EmplaceBackEfficiency) {
  tsfqueue::SPSCUnbounded<MockObject> q;
  MockObject::copies = 0;
  MockObject::moves = 0;

  q.emplace_back(42);

  EXPECT_EQ(MockObject::copies, 0);
  EXPECT_LE(MockObject::moves, 1);

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
  EXPECT_TRUE(is_pushable)
      << "Queue must support copy or move for push operations.";

  bool move_only_assignable = std::is_copy_assignable_v<MoveOnly> ||
                              std::is_move_assignable_v<MoveOnly>;
  EXPECT_TRUE(move_only_assignable)
      << "Move-only types should be allowed if they are move-assignable.";

  bool locked_type_assignable = std::is_copy_assignable_v<LockedType> ||
                                std::is_move_assignable_v<LockedType>;
  EXPECT_FALSE(locked_type_assignable)
      << "LockedType should fail the assignability requirement.";
}

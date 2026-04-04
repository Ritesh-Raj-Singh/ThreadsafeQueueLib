#ifndef FAST_LOCKFREE_SPSC_UNBOUNDED_BLOCK
#define FAST_LOCKFREE_SPSC_UNBOUNDED_BLOCK

#include "../utils.hpp"
#include "slots.hpp"
#include <atomic>

namespace tsfqueue::impl {
template <typename T> class Block_FAST {
private:
  using Slot_FAST = tsfqueue::FAST::Slot_FAST;
  std::size_t capacity;
  std::size_t mask;
  char *raw;
  char *data;
  std::unique_ptr<Slot_FAST> slots;
  template <typename U> void inner_enqueue(U &&data) {}

public:
  Block_FAST(std::size_t cap) {
    // Initialize by bringing it to the closest power of 2
    // And then make mask
    slots(static_cast<int>(cap));
  }
  ~Block_FAST() {
    // Make destructor
  }
  Block_FAST(const Block_FAST &other) = delete;
  Block_FAST &operator=(const Block_FAST &other) = delete;
  Block_FAST(Block_FAST &&other) {
    // Move constructor
  }
  template <typename U> bool try_enqueue(U &&data) {
    // Try to check if can push
    if (slots->try_get()) {
      inner_enqueue(std::forward<U>(data));
      return true;
    }
    return false;
  }
  template <typename U> void wait_enqueue(U &&data) {
    slots->wait_and_get();
    inner_enqueue(std::forward<U>(data));
  }
  template <typename U>
  bool wait_enqueue_timed(U &&data, std::int64_t time_usecs) {
    if (slots->timed_get(time_usecs)) {
      inner_enqueue(std::forward<U>(data));
      return true;
    }
    return false;
  }
};
} // namespace tsfqueue::impl

#endif
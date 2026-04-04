#ifndef FAST_LOCKFREE_SPSC_UNBOUNDED_SLOT
#define FAST_LOCKFREE_SPSC_UNBOUNDED_SLOT

#include "../utils.hpp"
#include <atomic>

namespace tsfqueue::FAST {
#if defined(__MACH__)
#include <mach/mach.h>
class Semaphore_FAST {
private:
  semaphore_t sema;
  Semaphore_FAST(const Semaphore &other) = delete;
  Semaphore_FAST(Semaphore &&other) = delete;

public:
  Semaphore_FAST(int count = 0) {
    assert(count >= 0);
    kern_return_t ret =
        semaphore_create(mach_task_self(), &sema, SYNC_POLICY_FIFO, count);
    assert(ret == KERN_SUCCESS);
  }
  ~Semaphore_FAST() { semaphore_destroy(mach_task_self(), sema); }
  bool try_get() { return timed_get(0); }
  bool timed_get(std::uint64_t time_usecs) {
    mach_timespec_t time;
    time.tv_sec = static_cast<unsigned int>(time_usecs / 1000000);
    time.tv_nsec = static_cast<int>((time_usecs % 1000000) * 1000);
    kern_return_t ret = semaphore_timedwait(sema, time);
    return ret == KERN_SUCCESS;
  }
  void wait_and_get() { semaphore_wait(sema); }
  void signal(int times = 1) {
    while (times--)
      while (semaphore_signal(sema) != KERN_SUCCESS)
        ;
  }
};
#endif

class Slot_FAST {
private:
  std::atomic<int> counter;
  Semaphore_FAST sema;
  static constexpr int SPIN_COUNT =
      1024; // Change this for benchmakrs & set to best possible
  inline bool hot_path() {
    for (int i = 0; i < SPIN_COUNT; i++) {
      if (counter.load(std::memory_order_acquire) > 0) {
        counter.fetch_sub(1);
        return true;
      }
    }
    return false;
  }
  bool get_with_sleep(std::int64_t time_usecs = -1) {
    if (hot_path())
      return true;
    counter.fetch_sub(1);
    if (time_usecs < 0) {
      sema.wait_and_get() return true;
    }
    if (sema.timed_get(static_cast<std::uint64_t>(time_usecs))) {
      return true;
    }

    // Restore the semaphore
    // Signal happened just after timeout expired
    // Thus we add a while loop which keeps checking until all boundary
    // conditions are gone. Super clever.
    while (true) {
      int old = counter.fetch_add(1);
      if (old < 0)
        return false; // Restored successfully
      old = counter.fetch_sub(1);
      if (old > 0 && sema.try_get()) {
        return true;
      }
    }
  }

public:
  Slot_FAST(int count) : counter(count), sema(0) {}
  bool try_get() {
    // Ok since SPSC so counter cannot be decremented between load and fetch_sub
    if (counter.load(std::memory_order_acquire) > 0) {
      counter.fetch_sub(1);
      return true;
    }
    return false;
  }
  bool timed_get(std::int64_t time_usecs) { return get_with_sleep(time_usecs); }
  void wait_and_get() { return get_with_sleep(); }
  void signal(int times = 1) {
    int old = counter.fetch_add(1);
    if (old < 0)
      sema.signal();
  }
}
}; // namespace tsfqueue::FAST

#endif
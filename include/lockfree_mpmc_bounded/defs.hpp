#ifndef LOCKFREE_MPMC_BOUNDED_DEFS
#define LOCKFREE_MPMC_BOUNDED_DEFS

#include "../utils.hpp"
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <thread>
#include <memory>

namespace tsfqueue::impl {

template <typename T, size_t N>
class lockfree_mpmc_bounded {
private:
    struct alignas(tsfqueue::impl::cache_line_size) Cell {
        std::atomic<size_t> sequence;
        T data;
    };

    static const size_t buffer_mask = N - 1;
    Cell buffer[N];
    
    alignas(tsfqueue::impl::cache_line_size) std::atomic<size_t> enqueue_pos;
    alignas(tsfqueue::impl::cache_line_size) std::atomic<size_t> dequeue_pos;

    static_assert((N >= 2) && ((N & (N - 1)) == 0), "N must be a power of 2");
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

public:
    lockfree_mpmc_bounded() {
        for (size_t i = 0; i < N; ++i) {
            buffer[i].sequence.store(i, std::memory_order_relaxed);
        }
        enqueue_pos.store(0, std::memory_order_relaxed);
        dequeue_pos.store(0, std::memory_order_relaxed);
    }

    ~lockfree_mpmc_bounded() = default;

    lockfree_mpmc_bounded(const lockfree_mpmc_bounded&) = delete;
    lockfree_mpmc_bounded& operator=(const lockfree_mpmc_bounded&) = delete;
    lockfree_mpmc_bounded(lockfree_mpmc_bounded&&) = delete;
    lockfree_mpmc_bounded& operator=(lockfree_mpmc_bounded&&) = delete;

    void push(T value);
    template <typename... Args> void emplace_back(Args&&... args);
    bool try_push(T value);
    template <typename... Args> bool try_emplace_back(Args&&... args);
    
    void wait_and_pop(T& value);
    bool try_pop(T& value);

    bool empty() const;
    size_t size() const;
};

} // namespace tsfqueue::impl

#endif

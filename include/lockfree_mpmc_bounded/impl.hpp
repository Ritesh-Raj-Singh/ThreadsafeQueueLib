#ifndef LOCKFREE_MPMC_BOUNDED_IMPL
#define LOCKFREE_MPMC_BOUNDED_IMPL

#include "defs.hpp"

namespace tsfqueue::impl {

template <typename T, size_t N>
template <typename... Args>
bool lockfree_mpmc_bounded<T, N>::try_emplace_back(Args&&... args) {
    Cell* cell;
    size_t pos = enqueue_pos.load(std::memory_order_relaxed);
    for (;;) {
        cell = &buffer[pos & buffer_mask];
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)pos;
        if (dif == 0) {
            if (enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                break;
            }
        } else if (dif < 0) {
            return false;
        } else {
            pos = enqueue_pos.load(std::memory_order_relaxed);
        }
    }
    cell->data = T(std::forward<Args>(args)...);
    cell->sequence.store(pos + 1, std::memory_order_release);
    return true;
}

template <typename T, size_t N>
bool lockfree_mpmc_bounded<T, N>::try_push(T value) {
    return try_emplace_back(std::move(value));
}

template <typename T, size_t N>
void lockfree_mpmc_bounded<T, N>::push(T value) {
    emplace_back(std::move(value));
}

template <typename T, size_t N>
template <typename... Args>
void lockfree_mpmc_bounded<T, N>::emplace_back(Args&&... args) {
    while (!try_emplace_back(std::forward<Args>(args)...)) {
        std::this_thread::yield();
    }
}

template <typename T, size_t N>
bool lockfree_mpmc_bounded<T, N>::try_pop(T& value) {
    Cell* cell;
    size_t pos = dequeue_pos.load(std::memory_order_relaxed);
    for (;;) {
        cell = &buffer[pos & buffer_mask];
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
        if (dif == 0) {
            if (dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                break;
            }
        } else if (dif < 0) {
            return false;
        } else {
            pos = dequeue_pos.load(std::memory_order_relaxed);
        }
    }
    value = std::move(cell->data);
    cell->sequence.store(pos + buffer_mask + 1, std::memory_order_release);
    return true;
}

template <typename T, size_t N>
void lockfree_mpmc_bounded<T, N>::wait_and_pop(T& value) {
    while (!try_pop(value)) {
        std::this_thread::yield();
    }
}

template <typename T, size_t N>
bool lockfree_mpmc_bounded<T, N>::empty() const {
    return size() == 0;
}

template <typename T, size_t N>
size_t lockfree_mpmc_bounded<T, N>::size() const {
    size_t push_idx = enqueue_pos.load(std::memory_order_acquire);
    size_t pop_idx = dequeue_pos.load(std::memory_order_acquire);
    if (push_idx > pop_idx) {
        return push_idx - pop_idx;
    }
    return 0;
}

} // namespace tsfqueue::impl

#endif

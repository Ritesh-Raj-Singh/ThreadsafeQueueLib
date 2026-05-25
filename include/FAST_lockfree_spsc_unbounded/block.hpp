#ifndef FAST_LOCKFREE_SPSC_UNBOUNDED_BLOCK
#define FAST_LOCKFREE_SPSC_UNBOUNDED_BLOCK

#include "../utils.hpp"
#include <atomic>
#include <cstddef>
#include <new>
#include <memory>
#include <utility>

namespace tsfqueue::impl {

template <typename T>
class Block_FAST {
public:
    static constexpr size_t capacity = 1024; // Fixed chunk size for simplicity and speed
    static constexpr size_t mask = capacity - 1;
    
    // We align the array to cache line to prevent false sharing with other fields
    alignas(tsfqueue::impl::cache_line_size) std::aligned_storage_t<sizeof(T), alignof(T)> data[capacity];
    
    alignas(tsfqueue::impl::cache_line_size) std::atomic<size_t> enqueue_pos{0};
    alignas(tsfqueue::impl::cache_line_size) std::atomic<size_t> dequeue_pos{0};
    
    alignas(tsfqueue::impl::cache_line_size) std::atomic<Block_FAST*> next{nullptr};

    Block_FAST() {}

    ~Block_FAST() {
        size_t head = dequeue_pos.load(std::memory_order_relaxed);
        size_t tail = enqueue_pos.load(std::memory_order_relaxed);
        for (size_t i = head; i < tail; ++i) {
            T* ptr = reinterpret_cast<T*>(&data[i & mask]);
            ptr->~T();
        }
    }

    Block_FAST(const Block_FAST&) = delete;
    Block_FAST& operator=(const Block_FAST&) = delete;

    template <typename... Args>
    bool try_enqueue(Args&&... args) {
        size_t pos = enqueue_pos.load(std::memory_order_relaxed);
        size_t head = dequeue_pos.load(std::memory_order_acquire);
        
        if (pos - head >= capacity) {
            return false; // Block is full
        }

        T* ptr = reinterpret_cast<T*>(&data[pos & mask]);
        new (ptr) T(std::forward<Args>(args)...);

        enqueue_pos.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool try_dequeue(T& out_val) {
        size_t pos = dequeue_pos.load(std::memory_order_relaxed);
        size_t tail = enqueue_pos.load(std::memory_order_acquire);

        if (pos == tail) {
            return false; // Block is empty
        }

        T* ptr = reinterpret_cast<T*>(&data[pos & mask]);
        out_val = std::move(*ptr);
        ptr->~T();

        dequeue_pos.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool is_empty() const {
        return dequeue_pos.load(std::memory_order_acquire) == enqueue_pos.load(std::memory_order_acquire);
    }
};

} // namespace tsfqueue::impl

#endif
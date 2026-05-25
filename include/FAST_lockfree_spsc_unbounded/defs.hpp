#ifndef FAST_LOCKFREE_SPSC_UNBOUNDED_DEFS
#define FAST_LOCKFREE_SPSC_UNBOUNDED_DEFS

#include "block.hpp"
#include "slots.hpp"
#include <atomic>
#include <type_traits>

namespace tsfqueue::impl {

template <typename T>
class fast_lockfree_spsc_unbounded {
private:
    alignas(tsfqueue::impl::cache_line_size) std::atomic<Block_FAST<T>*> head_block;
    alignas(tsfqueue::impl::cache_line_size) std::atomic<Block_FAST<T>*> tail_block;
    
    // We use Slot_FAST as an efficient way to block consumers when queue is empty
    tsfqueue::FAST::Slot_FAST elements;

    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

public:
    fast_lockfree_spsc_unbounded() : elements(0) {
        Block_FAST<T>* initial_block = new Block_FAST<T>();
        head_block.store(initial_block, std::memory_order_relaxed);
        tail_block.store(initial_block, std::memory_order_relaxed);
    }

    ~fast_lockfree_spsc_unbounded() {
        Block_FAST<T>* curr = head_block.load(std::memory_order_relaxed);
        while (curr) {
            Block_FAST<T>* next = curr->next.load(std::memory_order_relaxed);
            delete curr;
            curr = next;
        }
    }

    fast_lockfree_spsc_unbounded(const fast_lockfree_spsc_unbounded&) = delete;
    fast_lockfree_spsc_unbounded& operator=(const fast_lockfree_spsc_unbounded&) = delete;

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

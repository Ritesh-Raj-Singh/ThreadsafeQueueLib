#ifndef FAST_LOCKFREE_SPSC_UNBOUNDED_IMPL
#define FAST_LOCKFREE_SPSC_UNBOUNDED_IMPL

#include "defs.hpp"
#include <thread>

namespace tsfqueue::impl {

template <typename T>
template <typename... Args>
bool fast_lockfree_spsc_unbounded<T>::try_emplace_back(Args&&... args) {
    Block_FAST<T>* tail = tail_block.load(std::memory_order_relaxed);
    
    if (!tail->try_enqueue(std::forward<Args>(args)...)) {
        Block_FAST<T>* new_block = new Block_FAST<T>();
        new_block->try_enqueue(std::forward<Args>(args)...);
        
        tail->next.store(new_block, std::memory_order_release);
        tail_block.store(new_block, std::memory_order_relaxed);
    }
    
    elements.signal(1);
    return true;
}

template <typename T>
bool fast_lockfree_spsc_unbounded<T>::try_push(T value) {
    return try_emplace_back(std::move(value));
}

template <typename T>
void fast_lockfree_spsc_unbounded<T>::push(T value) {
    try_emplace_back(std::move(value));
}

template <typename T>
template <typename... Args>
void fast_lockfree_spsc_unbounded<T>::emplace_back(Args&&... args) {
    try_emplace_back(std::forward<Args>(args)...);
}

template <typename T>
bool fast_lockfree_spsc_unbounded<T>::try_pop(T& value) {
    if (!elements.try_get()) {
        return false;
    }

    Block_FAST<T>* head = head_block.load(std::memory_order_relaxed);
    
    if (head->try_dequeue(value)) {
        return true;
    }
    
    Block_FAST<T>* next_block = head->next.load(std::memory_order_acquire);
    if (next_block) {
        if (next_block->try_dequeue(value)) {
            head_block.store(next_block, std::memory_order_relaxed);
            delete head;
            return true;
        }
    }
    
    // Should never reach here if elements.try_get() succeeded, 
    // unless another thread popped, but this is SPSC.
    return false;
}

template <typename T>
void fast_lockfree_spsc_unbounded<T>::wait_and_pop(T& value) {
    elements.wait_and_get();

    Block_FAST<T>* head = head_block.load(std::memory_order_relaxed);
    
    if (head->try_dequeue(value)) {
        return;
    }
    
    Block_FAST<T>* next_block = head->next.load(std::memory_order_acquire);
    if (next_block) {
        if (next_block->try_dequeue(value)) {
            head_block.store(next_block, std::memory_order_relaxed);
            delete head;
            return;
        }
    }
}

template <typename T>
bool fast_lockfree_spsc_unbounded<T>::empty() const {
    // Actually empty could just check if the current block is empty and there's no next block
    Block_FAST<T>* head = head_block.load(std::memory_order_acquire);
    if (!head->is_empty()) return false;
    return head->next.load(std::memory_order_acquire) == nullptr;
}

template <typename T>
size_t fast_lockfree_spsc_unbounded<T>::size() const {
    size_t count = 0;
    Block_FAST<T>* curr = head_block.load(std::memory_order_acquire);
    while (curr) {
        size_t head_idx = curr->dequeue_pos.load(std::memory_order_acquire);
        size_t tail_idx = curr->enqueue_pos.load(std::memory_order_acquire);
        if (tail_idx > head_idx) {
            count += (tail_idx - head_idx);
        }
        curr = curr->next.load(std::memory_order_acquire);
    }
    return count;
}

} // namespace tsfqueue::impl

#endif

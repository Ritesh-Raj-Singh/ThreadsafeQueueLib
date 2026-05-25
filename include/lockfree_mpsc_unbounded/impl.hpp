#ifndef LOCKFREE_MPSC_UNBOUNDED_IMPL
#define LOCKFREE_MPSC_UNBOUNDED_IMPL

#include "defs.hpp"

namespace tsfqueue::impl {

template <typename T>
void lockfree_mpsc_unbounded<T>::push(T value) {
    emplace_back(std::move(value));
}

template <typename T>
template <typename... Args>
void lockfree_mpsc_unbounded<T>::emplace_back(Args&&... args) {
    node* new_node = new node(std::forward<Args>(args)...);
    node* old_tail = tail.exchange(new_node, std::memory_order_acq_rel);
    old_tail->next.store(new_node, std::memory_order_release);
    count.fetch_add(1, std::memory_order_relaxed);
}

template <typename T>
bool lockfree_mpsc_unbounded<T>::try_pop(T& value) {
    node* head_ptr = head.load(std::memory_order_relaxed);
    node* next_ptr = head_ptr->next.load(std::memory_order_acquire);

    if (next_ptr != nullptr) {
        value = std::move(next_ptr->data);
        head.store(next_ptr, std::memory_order_relaxed);
        delete head_ptr;
        count.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    node* tail_ptr = tail.load(std::memory_order_acquire);
    if (head_ptr != tail_ptr) {
        // A producer is in the middle of a push (has updated tail, but not old_tail->next)
        // Wait for them to finish linking
        while ((next_ptr = head_ptr->next.load(std::memory_order_acquire)) == nullptr) {
            std::this_thread::yield();
        }
        value = std::move(next_ptr->data);
        head.store(next_ptr, std::memory_order_relaxed);
        delete head_ptr;
        count.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    return false;
}

template <typename T>
void lockfree_mpsc_unbounded<T>::wait_and_pop(T& value) {
    while (!try_pop(value)) {
        std::this_thread::yield();
    }
}

template <typename T>
bool lockfree_mpsc_unbounded<T>::empty() const {
    node* head_ptr = head.load(std::memory_order_relaxed);
    node* next_ptr = head_ptr->next.load(std::memory_order_acquire);
    if (next_ptr != nullptr) return false;
    node* tail_ptr = tail.load(std::memory_order_acquire);
    return head_ptr == tail_ptr;
}

template <typename T>
size_t lockfree_mpsc_unbounded<T>::size() const {
    return count.load(std::memory_order_relaxed);
}

} // namespace tsfqueue::impl

#endif

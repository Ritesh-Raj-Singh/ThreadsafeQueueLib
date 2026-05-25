#ifndef LOCKFREE_MPSC_UNBOUNDED_DEFS
#define LOCKFREE_MPSC_UNBOUNDED_DEFS

#include "../utils.hpp"
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <memory>
#include <thread>

namespace tsfqueue::impl {

template <typename T>
class lockfree_mpsc_unbounded {
private:
    struct node {
        T data;
        std::atomic<node*> next;
        node() : next(nullptr) {}
        node(T&& val) : data(std::move(val)), next(nullptr) {}
        template<typename... Args>
        node(Args&&... args) : data(std::forward<Args>(args)...), next(nullptr) {}
    };

    alignas(cache_line_size) std::atomic<node*> head;
    alignas(cache_line_size) std::atomic<node*> tail;
    alignas(cache_line_size) std::atomic<size_t> count{0};

    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

public:
    lockfree_mpsc_unbounded() {
        node* stub = new node();
        head.store(stub, std::memory_order_relaxed);
        tail.store(stub, std::memory_order_relaxed);
    }

    ~lockfree_mpsc_unbounded() {
        node* current = head.load(std::memory_order_relaxed);
        while (current != nullptr) {
            node* next_node = current->next.load(std::memory_order_relaxed);
            delete current;
            current = next_node;
        }
    }

    lockfree_mpsc_unbounded(const lockfree_mpsc_unbounded&) = delete;
    lockfree_mpsc_unbounded& operator=(const lockfree_mpsc_unbounded&) = delete;
    lockfree_mpsc_unbounded(lockfree_mpsc_unbounded&&) = delete;
    lockfree_mpsc_unbounded& operator=(lockfree_mpsc_unbounded&&) = delete;

    void push(T value);
    template <typename... Args> void emplace_back(Args&&... args);
    bool try_pop(T& value);
    void wait_and_pop(T& value);
    bool empty() const;
    size_t size() const;
};

} // namespace tsfqueue::impl

#endif

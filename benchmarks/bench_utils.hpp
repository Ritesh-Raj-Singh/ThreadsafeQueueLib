#ifndef BENCH_UTILS_HPP
#define BENCH_UTILS_HPP

#include <type_traits>
#include <thread>

// Detection traits
template<typename Q, typename T, typename = void>
struct has_try_push : std::false_type {};
template<typename Q, typename T>
struct has_try_push<Q, T, std::void_t<decltype(std::declval<Q>().try_push(std::declval<T>()))>> : std::true_type {};

template<typename Q, typename T, typename = void>
struct has_try_enqueue : std::false_type {};
template<typename Q, typename T>
struct has_try_enqueue<Q, T, std::void_t<decltype(std::declval<Q>().try_enqueue(std::declval<T>()))>> : std::true_type {};

template<typename Q, typename T, typename = void>
struct has_try_pop : std::false_type {};
template<typename Q, typename T>
struct has_try_pop<Q, T, std::void_t<decltype(std::declval<Q>().try_pop(std::declval<T&>()))>> : std::true_type {};

template<typename Q, typename T, typename = void>
struct has_try_dequeue : std::false_type {};
template<typename Q, typename T>
struct has_try_dequeue<Q, T, std::void_t<decltype(std::declval<Q>().try_dequeue(std::declval<T&>()))>> : std::true_type {};

template <typename Q, typename T>
void enqueue(Q& q, T val) {
    if constexpr (has_try_enqueue<Q, T>::value) {
        while (!q.try_enqueue(val)) { std::this_thread::yield(); }
    } else if constexpr (has_try_push<Q, T>::value) {
        while (!q.try_push(val)) { std::this_thread::yield(); }
    } else {
        q.push(val);
    }
}

template <typename Q, typename T>
bool dequeue(Q& q, T& val) {
    if constexpr (has_try_dequeue<Q, T>::value) {
        return q.try_dequeue(val);
    } else if constexpr (has_try_pop<Q, T>::value) {
        return q.try_pop(val);
    } else {
        // Rigtorp uses front() and pop()
        auto* ptr = q.front();
        if (ptr) {
            val = *ptr;
            q.pop();
            return true;
        }
        return false;
    }
}

#endif

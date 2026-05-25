#ifndef BENCH_UTILS_HPP
#define BENCH_UTILS_HPP

#include <type_traits>
#include <thread>

template<typename Q, typename T, typename = void>
struct has_try_push : std::false_type {};

template<typename Q, typename T>
struct has_try_push<Q, T, std::void_t<decltype(std::declval<Q>().try_push(std::declval<T>()))>> : std::true_type {};

template <typename Q, typename T>
void enqueue(Q& q, T val) {
    if constexpr (has_try_push<Q, T>::value) {
        while (!q.try_push(val)) { std::this_thread::yield(); }
    } else {
        q.push(val);
    }
}

#endif

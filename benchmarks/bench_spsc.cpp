#include <benchmark/benchmark.h>
#include <thread>
#include "tsfqueue.hpp"
#include "bench_utils.hpp"

template <typename Queue>
static void BM_SPSC(benchmark::State& state) {
    const int ops = state.range(0);
    for (auto _ : state) {
        Queue q;
        std::thread producer([&]() {
            for (int i = 0; i < ops; ++i) {
                enqueue(q, i);
            }
        });
        std::thread consumer([&]() {
            for (int i = 0; i < ops; ++i) {
                int val;
                while (!q.try_pop(val)) { std::this_thread::yield(); }
            }
        });
        producer.join();
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * ops);
}

// Bounded SPSC requires N at compile time
static void BM_SPSCBounded(benchmark::State& state) {
    BM_SPSC<tsfqueue::SPSCBounded<int, 65536>>(state);
}
BENCHMARK(BM_SPSCBounded)->RangeMultiplier(10)->Range(1000, 1000000)->UseRealTime();

static void BM_SPSCUnbounded(benchmark::State& state) {
    BM_SPSC<tsfqueue::SPSCUnbounded<int>>(state);
}
BENCHMARK(BM_SPSCUnbounded)->RangeMultiplier(10)->Range(1000, 1000000)->UseRealTime();

static void BM_FastSPSCUnbounded(benchmark::State& state) {
    BM_SPSC<tsfqueue::FastSPSCUnbounded<int>>(state);
}
BENCHMARK(BM_FastSPSCUnbounded)->RangeMultiplier(10)->Range(1000, 1000000)->UseRealTime();

BENCHMARK_MAIN();

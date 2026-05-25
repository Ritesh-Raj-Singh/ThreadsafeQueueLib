#include <benchmark/benchmark.h>
#include <thread>
#include "tsfqueue.hpp"
#include "bench_utils.hpp"
#include "readerwriterqueue.h"
#include <rigtorp/SPSCQueue.h>

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
                while (!dequeue(q, val)) { std::this_thread::yield(); }
                benchmark::DoNotOptimize(val);
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

static void BM_MoodycamelSPSC(benchmark::State& state) {
    BM_SPSC<moodycamel::ReaderWriterQueue<int>>(state);
}
BENCHMARK(BM_MoodycamelSPSC)->RangeMultiplier(10)->Range(1000, 1000000)->UseRealTime();

template <typename T>
struct RigtorpWrapper {
    rigtorp::SPSCQueue<T> q{65536};
    bool try_push(const T& val) { return q.try_push(val); }
    T* front() { return q.front(); }
    void pop() { q.pop(); }
};

static void BM_RigtorpSPSC(benchmark::State& state) {
    BM_SPSC<RigtorpWrapper<int>>(state);
}
BENCHMARK(BM_RigtorpSPSC)->RangeMultiplier(10)->Range(1000, 1000000)->UseRealTime();

BENCHMARK_MAIN();

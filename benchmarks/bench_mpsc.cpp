#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <atomic>
#include "tsfqueue.hpp"
#include "bench_utils.hpp"

template <typename Queue>
static void BM_MPSC(benchmark::State& state) {
    const int num_producers = state.range(0);
    const int ops_per_producer = 100000;
    const int total_ops = num_producers * ops_per_producer;

    for (auto _ : state) {
        Queue q;
        std::vector<std::thread> producers;
        for (int p = 0; p < num_producers; ++p) {
            producers.emplace_back([&q, ops_per_producer]() {
                for (int i = 0; i < ops_per_producer; ++i) {
                    enqueue(q, i);
                }
            });
        }
        
        std::thread consumer([&q, total_ops]() {
            for (int i = 0; i < total_ops; ++i) {
                int val;
                while (!q.try_pop(val)) { std::this_thread::yield(); }
            }
        });

        for (auto& t : producers) t.join();
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * total_ops);
}

// Bounded MPMC can act as MPSC
static void BM_MPSC_MPMCBounded(benchmark::State& state) {
    BM_MPSC<tsfqueue::MPMCBounded<int, 65536>>(state);
}
BENCHMARK(BM_MPSC_MPMCBounded)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime();

static void BM_MPSC_MPSCUnbounded(benchmark::State& state) {
    BM_MPSC<tsfqueue::MPSCUnbounded<int>>(state);
}
BENCHMARK(BM_MPSC_MPSCUnbounded)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime();

static void BM_MPSC_BlockingMPMCUnbounded(benchmark::State& state) {
    BM_MPSC<tsfqueue::BlockingMPMCUnbounded<int>>(state);
}
BENCHMARK(BM_MPSC_BlockingMPMCUnbounded)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime();

BENCHMARK_MAIN();

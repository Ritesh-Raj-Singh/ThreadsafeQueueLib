#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <atomic>
#include "tsfqueue.hpp"
#include "bench_utils.hpp"

template <typename Queue>
static void BM_MPMC(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int ops_per_thread = 100000;
    const int total_ops = num_threads * ops_per_thread;

    for (auto _ : state) {
        Queue q;
        std::atomic<int> total_popped{0};
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;
        
        for (int p = 0; p < num_threads; ++p) {
            producers.emplace_back([&q, ops_per_thread]() {
                for (int i = 0; i < ops_per_thread; ++i) {
                    enqueue(q, i);
                }
            });
        }
        
        for (int c = 0; c < num_threads; ++c) {
            consumers.emplace_back([&q, &total_popped, total_ops]() {
                while (total_popped.load(std::memory_order_relaxed) < total_ops) {
                    int val;
                    if (q.try_pop(val)) {
                        total_popped.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }

        for (auto& t : producers) t.join();
        for (auto& t : consumers) t.join();
    }
    state.SetItemsProcessed(state.iterations() * total_ops);
}

static void BM_MPMC_MPMCBounded(benchmark::State& state) {
    BM_MPMC<tsfqueue::MPMCBounded<int, 65536>>(state);
}
BENCHMARK(BM_MPMC_MPMCBounded)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime();

static void BM_MPMC_BlockingMPMCUnbounded(benchmark::State& state) {
    BM_MPMC<tsfqueue::BlockingMPMCUnbounded<int>>(state);
}
BENCHMARK(BM_MPMC_BlockingMPMCUnbounded)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime();

BENCHMARK_MAIN();

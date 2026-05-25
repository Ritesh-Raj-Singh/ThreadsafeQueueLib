# ThreadsafeQueueLib

**ThreadsafeQueueLib** is a high-performance, lightweight header-only C++17 library providing robust thread-safe queues. Designed for flexibility and extreme concurrency, it features specialized wait-free, lock-free, and blocking queues tailored for Single-Producer/Single-Consumer (SPSC), Multi-Producer/Single-Consumer (MPSC), and Multi-Producer/Multi-Consumer (MPMC) paradigms.

Whether you need strict boundary allocations, high-throughput dynamic allocations, or block-based chunking mechanisms, `ThreadsafeQueueLib` gives you fine-grained control over your thread-synchronization primitives.

---

## Supported Queues

All queues are easily accessible by including a single header file:
```cpp
#include <tsfqueue.hpp>
```

### 1. Single-Producer Single-Consumer (SPSC)
Ideal for data pipelining between two distinct threads without lock contention.
*   **`tsfqueue::SPSCBounded<T, N>`**: A bounded lock-free queue that pre-allocates an array of `N` elements. Perfect for embedded systems or high-performance scenarios where dynamic memory allocation is strictly prohibited.
*   **`tsfqueue::SPSCUnbounded<T>`**: An unbounded lock-free queue utilizing a node-based linked list. It dynamically allocates memory as needed, preventing the queue from ever blocking a producer.
*   **`tsfqueue::FastSPSCUnbounded<T>`**: A highly optimized unbounded queue that allocates items in "chunks" or "blocks" of 1024 elements at a time. This drastically reduces the overhead of dynamic allocation compared to the standard unbounded version while remaining entirely lock-free on the enqueue path.

### 2. Multi-Producer Single-Consumer (MPSC)
Designed for many-to-one scenarios, such as multiple worker threads pushing results to a single aggregator or logger thread.
*   **`tsfqueue::MPSCUnbounded<T>`**: An unbounded lock-free linked-list queue. Multiple producers safely synchronize using atomic swaps on the tail, while the single consumer pops from the head seamlessly.

### 3. Multi-Producer Multi-Consumer (MPMC)
The most versatile queues, suitable for thread pools and complex task distribution.
*   **`tsfqueue::MPMCBounded<T, N>`**: A pure lock-free, wait-free bounded queue. It utilizes a pre-allocated array of "Cells" containing sequence numbers. Multiple producers and consumers use `compare_exchange_weak` to safely and quickly claim slots without using mutex locks. `N` must be a power of 2.
*   **`tsfqueue::BlockingMPMCUnbounded<T>`**: An unbounded queue utilizing standard C++ `std::mutex` and `std::condition_variable`. While not lock-free, it provides an exceptionally stable and reliable fallback when absolute raw latency is less critical than CPU-saving sleep states.

---

## Usage Example

```cpp
#include "tsfqueue.hpp"
#include <iostream>
#include <thread>

int main() {
    // Instantiate a lock-free bounded SPSC queue with capacity 1024
    tsfqueue::SPSCBounded<int, 1024> queue;

    std::thread producer([&queue]() {
        for (int i = 0; i < 100; ++i) {
            // Spin-wait until successful
            while (!queue.try_push(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&queue]() {
        for (int i = 0; i < 100; ++i) {
            int value;
            while (!queue.try_pop(value)) {
                std::this_thread::yield();
            }
            std::cout << "Popped: " << value << "\n";
        }
    });

    producer.join();
    consumer.join();
    return 0;
}
```

---

## Testing & Verification

The repository contains an exhaustive test suite utilizing **Google Test (GTest)**. The tests stress-test the concurrent nature of the queues under heavy load to guarantee race-condition-free operation and lock-free integrity.

### Prerequisites
*   CMake (3.14 or higher)
*   A C++17 compatible compiler (GCC, Clang, or MSVC)

### Build and Run Tests & Benchmarks
Clone the repository and run the following commands to configure, build, and execute the test suite alongside Google Benchmarks:

```bash
# 1. Generate the build files
mkdir -p build && cd build
cmake ..

# 2. Compile the executables
make -j$(nproc)

# 3. Run all tests via CTest
ctest --output-on-failure
```

If you wish to test a specific queue configuration, you can execute the compiled test binaries directly:
```bash
./test_spsc
./test_mpsc
./test_mpmc
```

## Benchmark Results

The library uses **Google Benchmark** to accurately measure queue throughput (Operations Per Second). You can run the benchmarks yourself after building:
```bash
./bench_spsc
./bench_mpsc
./bench_mpmc
```

**Key Findings:**
- **SPSC Bounded**: Achieves **~15.6 Million ops/sec**, providing extreme speed with zero memory allocation overhead.
- **MPSC Unbounded**: Phenomenal scaling; throughput *increases* under contention (from 3.0M ops/sec with 1 producer to **6.1M ops/sec** with 8 producers) due to efficient atomic tail swaps.
- **MPMC Bounded**: Sustains robust throughput (**8.9M ops/sec** 1v1, **2.6M ops/sec** under brutal 8v8 contention), easily outperforming traditional mutex-based queues which struggle to exceed 1M ops/sec.

---

Check out the `examples/` directory for ready-to-run use cases. We have compiled a detailed explanation of each architectural pattern in the [`examples/examples.md`](examples/examples.md) file.

Included examples:
1. `01_spsc_logger.cpp` - A high-performance background logging thread using `FastSPSCUnbounded`.
2. `02_mpmc_threadpool.cpp` - A multi-threaded task pool architecture using `MPMCBounded`.
3. `03_mpsc_event_aggregator.cpp` - A lock-free event/metrics collector using `MPSCUnbounded`.
4. `04_spscb_audio_buffer.cpp` - A zero-allocation audio stream pipeline using `SPSCBounded`.
# ThreadsafeQueueLib Examples

This directory contains real-world use cases and architectural examples demonstrating how to properly leverage the various queues provided by `ThreadsafeQueueLib`.

All examples are automatically built when you compile the project using CMake. You can find their executables in your `build/` directory (e.g., `./example_spsc_logger`).

---

## 1. Background Logger (`01_spsc_logger.cpp`)
**Queue Used:** `FastSPSCUnbounded<T>`

**Description:**
Logging is a classic use case for a Single-Producer, Single-Consumer (SPSC) queue. In high-performance applications (like game engines or high-frequency trading), the main thread cannot afford to block while writing strings to `stdout` or a file.
This example uses the `FastSPSCUnbounded` queue to pass log strings from the main thread to a dedicated background logging thread. Because the queue is unbounded, the main thread will never block even if the logger falls behind, and the chunked memory allocation ensures the overhead of pushing strings remains practically zero.

## 2. Worker Thread Pool (`02_mpmc_threadpool.cpp`)
**Queue Used:** `MPMCBounded<T, N>`

**Description:**
A standard thread pool pattern requires a Multi-Producer, Multi-Consumer (MPMC) queue so that any thread can submit a task and any available worker thread can claim it.
This example demonstrates submitting `std::function<void()>` objects to the lock-free, wait-free `MPMCBounded` queue. By using a bounded queue, we inherently limit the backlog of pending tasks, ensuring memory stability under heavy load while taking advantage of the absolute lowest latency task distribution possible.

## 3. Metrics/Event Aggregator (`03_mpsc_event_aggregator.cpp`)
**Queue Used:** `MPSCUnbounded<T>`

**Description:**
In distributed or multi-threaded architectures, you often have multiple worker threads generating events, metrics, or completed sub-tasks that need to be collected and processed by a single central thread (like a database writer or a GUI event loop).
This Multi-Producer, Single-Consumer (MPSC) example shows multiple threads safely pushing `Event` structs into the `MPSCUnbounded` queue. The lock-free nature ensures that the worker threads never block each other, even when pushing simultaneously.

## 4. Audio Pipeline Buffer (`04_spscb_audio_buffer.cpp`)
**Queue Used:** `SPSCBounded<T, N>`

**Description:**
In low-level audio processing, a producer thread renders audio frames and pushes them to a hardware DAC consumer thread. In these environments, **dynamic memory allocation is strictly forbidden** because a page-fault or malloc-lock would cause an audible glitch (underrun).
This example uses `SPSCBounded` because it pre-allocates all its memory at compile time (or construction time). The audio producer spins to push frames, and the consumer flawlessly pops them without ever touching the OS memory allocator.

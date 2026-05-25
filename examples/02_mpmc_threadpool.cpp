#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include "tsfqueue.hpp"

// A simple Thread Pool implementation using the wait-free MPMCBounded queue.
// Multiple producers can submit tasks, and multiple worker threads consume them.

int main() {
    // A queue of function objects (tasks). Capacity must be a power of 2.
    tsfqueue::MPMCBounded<std::function<void()>, 1024> task_queue;
    std::atomic<bool> stop_flag{false};

    const int num_workers = 4;
    std::vector<std::thread> workers;

    // Start Worker Threads (Consumers)
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back([&task_queue, &stop_flag, i]() {
            std::function<void()> task;
            while (!stop_flag.load()) {
                if (task_queue.try_pop(task)) {
                    // Execute the task
                    task();
                } else {
                    // Yield if no tasks are available
                    std::this_thread::yield();
                }
            }
            // Drain remaining tasks after stop flag is set
            while (task_queue.try_pop(task)) {
                task();
            }
        });
    }

    // Submit Tasks (Producers)
    // We can simulate multiple producers by just pushing tasks in a loop here,
    // but imagine this happening across multiple threads in a real application.
    for (int i = 1; i <= 20; ++i) {
        auto task = [i]() {
            // Task just prints its ID safely
            std::cout << "Worker executed task: " << i << "\n";
        };

        // If the bounded queue is full, try_push returns false, so we yield and retry.
        while (!task_queue.try_push(task)) {
            std::this_thread::yield();
        }
    }

    // Give workers time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Shutdown
    stop_flag.store(true);
    for (auto& worker : workers) {
        worker.join();
    }

    std::cout << "Thread pool finished executing all tasks.\n";
    return 0;
}

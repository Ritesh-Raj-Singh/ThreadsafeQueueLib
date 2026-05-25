#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "tsfqueue.hpp"

// A simple thread-safe logger using a Fast SPSC queue.
// The main thread produces log messages, and a dedicated background thread consumes and prints them.

int main() {
    tsfqueue::FastSPSCUnbounded<std::string> log_queue;
    std::atomic<bool> running{true};

    // Consumer thread (Logger)
    std::thread logger_thread([&]() {
        std::string msg;
        while (running.load() || !log_queue.empty()) {
            // Using try_pop for non-blocking pop
            if (log_queue.try_pop(msg)) {
                std::cout << "[LOG]: " << msg << "\n";
            } else {
                // Yield to prevent pegging the CPU when queue is empty
                std::this_thread::yield(); 
            }
        }
    });

    // Producer (Main Thread)
    for (int i = 1; i <= 10; ++i) {
        std::string log_msg = "Processing task " + std::to_string(i);
        log_queue.push(log_msg);
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Shutdown
    log_queue.push("Shutting down logger...");
    running.store(false);
    logger_thread.join();

    std::cout << "Application finished cleanly.\n";
    return 0;
}

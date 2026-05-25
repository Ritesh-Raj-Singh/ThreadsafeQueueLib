#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include "tsfqueue.hpp"

// A simple Event Aggregator demonstrating the MPSC (Multi-Producer, Single-Consumer) queue.
// Multiple worker threads (producers) generate events or metrics.
// A single aggregator thread (consumer) collects and processes them efficiently.

struct Event {
    int worker_id;
    std::string message;
    long long timestamp_ms;
};

int main() {
    tsfqueue::MPSCUnbounded<Event> event_queue;
    std::atomic<bool> running{true};
    
    // Aggregator (Consumer Thread)
    std::thread aggregator([&]() {
        int total_events = 0;
        Event ev;
        
        while (running.load() || !event_queue.empty()) {
            if (event_queue.try_pop(ev)) {
                total_events++;
                std::cout << "[Aggregator] Received from Worker " << ev.worker_id 
                          << ": " << ev.message << " (Total: " << total_events << ")\n";
            } else {
                std::this_thread::yield();
            }
        }
        std::cout << "[Aggregator] Finished processing. Total events: " << total_events << "\n";
    });

    // Workers (Producer Threads)
    const int num_workers = 3;
    std::vector<std::thread> workers;
    
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back([&event_queue, i]() {
            for (int e = 1; e <= 5; ++e) {
                // Simulate varying work times
                std::this_thread::sleep_for(std::chrono::milliseconds(20 * (i + 1)));
                
                auto now = std::chrono::system_clock::now().time_since_epoch();
                long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
                
                // Push an event safely without locks
                event_queue.push(Event{i, "Completed subtask " + std::to_string(e), ms});
            }
        });
    }

    // Wait for producers to finish
    for (auto& w : workers) {
        w.join();
    }
    
    // Signal aggregator to shutdown and wait
    running.store(false);
    aggregator.join();

    return 0;
}

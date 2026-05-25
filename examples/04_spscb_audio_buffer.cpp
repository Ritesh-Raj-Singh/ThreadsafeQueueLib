#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "tsfqueue.hpp"

// A mock Audio Streaming Buffer demonstrating the SPSCBounded queue.
// In audio pipelines (like rendering waveforms to a soundcard), latency is critical
// and dynamic memory allocation is strictly prohibited. Bounded SPSC queues are perfect for this.

struct AudioFrame {
    float samples[64]; // Small block of audio
};

int main() {
    // A fixed-size bounded queue for passing audio frames without any dynamic allocation overhead.
    tsfqueue::SPSCBounded<AudioFrame, 512> audio_buffer;
    std::atomic<bool> stream_active{true};
    
    // Audio Hardware (Consumer Thread)
    // Simulates an audio DAC pulling frames at exactly real-time intervals.
    std::thread audio_hardware([&]() {
        int frames_played = 0;
        int buffer_underruns = 0;
        AudioFrame frame;
        
        while (stream_active.load() || !audio_buffer.empty()) {
            if (audio_buffer.try_pop(frame)) {
                frames_played++;
                // Simulate audio consumption (e.g., 44.1kHz playback rate)
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            } else {
                if (stream_active.load()) {
                    buffer_underruns++; // The producer is too slow!
                }
                std::this_thread::yield();
            }
        }
        std::cout << "[Hardware] Finished. Frames played: " << frames_played 
                  << ", Underruns: " << buffer_underruns << "\n";
    });

    // Audio Renderer (Producer Thread)
    std::thread audio_renderer([&]() {
        AudioFrame empty_frame{};
        
        for (int i = 0; i < 1000; ++i) {
            // Render an audio frame (simulated)
            
            // Try to push. If the bounded buffer is full, the audio producer must wait.
            // Using a spin wait to avoid OS scheduler latency.
            while (!audio_buffer.try_push(empty_frame)) {
                std::this_thread::yield();
            }
        }
        stream_active.store(false);
    });

    audio_renderer.join();
    audio_hardware.join();

    return 0;
}

#include "PipelineManager.hpp"
#include "ZmqHandler.hpp"
#include <iostream>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t running = 1;

void signal_handler(int) {
    running = 0;
}

int main() {
    gst_init(nullptr, nullptr);

    PipelineManager manager;
    ZmqHandler handler("tcp://*:5555", "command.pipeline", [&](json_object* params) {
        const char* pipeline = json_object_get_string(json_object_object_get(params, "pipeline"));
        const char* transport = json_object_get_string(json_object_object_get(params, "transport"));
        const char* streamId = json_object_get_string(json_object_object_get(params, "streamId"));

        if (pipeline && transport && streamId) {
            manager.start_pipeline(pipeline, transport, streamId);
        }
    });

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    handler.start();

    std::cout << "Worker running...\n";

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        manager.cleanup();
    }

    return 0;
}
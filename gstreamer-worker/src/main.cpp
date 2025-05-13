#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <gst/gst.h>
#include <signal.h>
#include "PipelineManager.hpp"
#include "ZmqHandler.hpp"

volatile sig_atomic_t running = 1;

void signal_handler(int) {
    running = 0;
}

using namespace std::placeholders;
void handle_rpc_request(json_object* params)
{
    // Extract required fields from JSON
    json_object* pipeline_obj = json_object_object_get(params, "pipeline");
    json_object* transport_obj = json_object_object_get(params, "transport");
    json_object* stream_id_obj = json_object_object_get(params, "streamId");

    if (!pipeline_obj || !json_object_is_type(pipeline_obj, json_type_string) ||
        !transport_obj || !json_object_is_type(transport_obj, json_type_string) ||
        !stream_id_obj || !json_object_is_type(stream_id_obj, json_type_string))
    {
        log_message("error", "Missing or invalid parameters in RPC request");
        return;
    }

    const char* pipeline_str = json_object_get_string(pipeline_obj);
    const char* transport_str = json_object_get_string(transport_obj);
    const char* stream_id_str = json_object_get_string(stream_id_obj);

    // ✅ Call create_pipeline_context()
    auto ctx = std::make_shared<PipelineContext>(pipeline_str, transport_str, stream_id_str);

    if (ctx->get_pipeline())
    {
        GstStateChangeReturn ret = gst_element_set_state(ctx->get_pipeline(), GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE)
        {
            log_message("error", "Failed to start pipeline for %s", stream_id_str);
        }
        else
        {
            log_message("info", "Pipeline started successfully for stream: %s", stream_id_str);
        }
    }
    else
    {
        log_message("error", "Failed to create pipeline for stream: %s", stream_id_str);
    }
}

int main() {
    gst_init(nullptr, nullptr);
    log_message("info", "Initializing GStreamer worker...");

    check_hardware_acceleration();
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    PipelineManager manager;
    ZmqHandler handler("tcp://*:5555", "command.pipeline", [&](json_object* params) {
        const char* pipeline = json_object_get_string(json_object_object_get(params, "pipeline"));
        const char* transport = json_object_get_string(json_object_object_get(params, "transport"));
        const char* streamId = json_object_get_string(json_object_object_get(params, "streamId"));

        if (pipeline && transport && streamId) {
            manager.start_pipeline(pipeline, transport, streamId);
        }
    });

    handler.start();

    std::cout << "Worker running...\n";

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        manager.cleanup();
    }

    return 0;
}
#include "PipelineManager.hpp"
#include <iostream>
#include <chrono>
#include <thread>

void PipelineManager::start_pipeline(
    const std::string& pipeline,
    const std::string& transport,
    const std::string& stream_id
) {
    auto ctx = ::create_pipeline_context(pipeline, transport, stream_id, "/tmp/gst_shm_" + stream_id);
    if (!ctx) return;

    std::unique_lock lock(mtx_);
    pipelines_[stream_id] = ctx;

    // Launch in background thread
    std::thread([ctx]() {
        GstStateChangeReturn ret = gst_element_set_state(ctx->pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            std::cerr << "Failed to start pipeline for stream: " << ctx->stream_id << "\n";
        }
    }).detach();
}

void PipelineManager::stop_pipeline(const std::string& stream_id) {
    std::unique_lock lock(mtx_);
    auto it = pipelines_.find(stream_id);
    if (it != pipelines_.end()) {
        gst_element_set_state(it->second->pipeline, GST_STATE_NULL);
        pipelines_.erase(it);
    }
}

void PipelineManager::cleanup() {
    std::unique_lock lock(mtx_);
    for (auto& [id, ctx] : pipelines_) {
        GstState state;
        gst_element_get_state(ctx->pipeline, &state, nullptr, GST_CLOCK_TIME_NONE);
        if (state == GST_STATE_NULL) {
            pipelines_.erase(id);
        }
    }
}
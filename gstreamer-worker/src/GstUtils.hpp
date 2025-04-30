#pragma once

#include <gst/gst.h>
#include <string>
#include <memory>

struct PipelineContext {
    std::string stream_id;
    GstElement* pipeline = nullptr;

    ~PipelineContext() {
        if (pipeline) gst_object_unref(pipeline);
    }
};

std::shared_ptr<PipelineContext> create_pipeline_context(
    const std::string& pipeline_desc,
    const std::string& transport,
    const std::string& stream_id,
    const std::string& shm_path = "/tmp/gst_shm_video_001"
);
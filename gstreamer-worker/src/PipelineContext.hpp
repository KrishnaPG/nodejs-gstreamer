// PipelineContext.hpp
#pragma once

#include <string>
#include <memory>
#include <gst/gst.h>

class PipelineContext
{
public:
    explicit PipelineContext(const std::string& pipeline_desc,
        const std::string& transport,
        const std::string& stream_id,
        const std::string& shm_path = "/tmp/gst_shm_video_001");

    ~PipelineContext();

    GstElement* get_pipeline() const { return pipeline; }
    const std::string& get_stream_id() const { return stream_id; }

private:
    GstElement* pipeline = nullptr;
    std::string stream_id;
};
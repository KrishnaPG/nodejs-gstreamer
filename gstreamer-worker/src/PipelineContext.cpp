// PipelineContext.cpp
#include "PipelineContext.hpp"
#include "Logger.hpp"
#include <json-c/json.h>

PipelineContext::PipelineContext(const std::string& pipeline_desc,
    const std::string& transport,
    const std::string& stream_id,
    const std::string& shm_path) : stream_id(stream_id)
{

    std::string modified_pipeline;

    if (transport == "shm")
    {
        modified_pipeline = pipeline_desc + " ! shmsink socket-path=" +
            shm_path + " wait-for-connection=false";
    }
    else if (transport == "zeromq")
    {
        modified_pipeline = pipeline_desc + " ! zmqsink host=tcp://127.0.0.1:5556 topic=" +
            stream_id;
    }
    else
    {
        log_message("error", "Unsupported transport: %s", transport.c_str());
        return;
    }

    GError* error = nullptr;
    pipeline = gst_parse_launch(modified_pipeline.c_str(), &error);

    if (!pipeline)
    {
        log_message("error", "Failed to parse pipeline: %s", error->message);
        g_error_free(error);
    }
}

PipelineContext::~PipelineContext()
{
    if (pipeline) gst_object_unref(pipeline);
}
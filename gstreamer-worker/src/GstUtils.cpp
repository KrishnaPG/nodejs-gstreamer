#include "GstUtils.hpp"
#include <iostream>

std::shared_ptr<PipelineContext> create_pipeline_context(
    const std::string& pipeline_desc,
    const std::string& transport,
    const std::string& stream_id,
    const std::string& shm_path
) {
    auto ctx = std::make_shared<PipelineContext>();
    ctx->stream_id = stream_id;

    std::string final_pipeline = pipeline_desc;

    if (transport == "shm") {
        final_pipeline += " ! shmsink socket-path=" + shm_path +
                          " wait-for-connection=false";
    } else if (transport == "zeromq") {
        final_pipeline += " ! zmqsink host=tcp://127.0.0.1:5556 topic=" + stream_id;
    } else {
        std::cerr << "Unsupported transport: " << transport << "\n";
        return nullptr;
    }

    GError* error = nullptr;
    ctx->pipeline = gst_parse_launch(final_pipeline.c_str(), &error);

    if (!ctx->pipeline) {
        std::cerr << "Failed to parse pipeline: " << error->message << "\n";
        g_error_free(error);
        return nullptr;
    }

    return ctx;
}
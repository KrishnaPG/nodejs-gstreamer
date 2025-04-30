#pragma once

#include "GstUtils.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <thread>
#include <vector>

class PipelineManager {
public:
    void start_pipeline(const std::string& pipeline,
                        const std::string& transport,
                        const std::string& stream_id);

    void stop_pipeline(const std::string& stream_id);
    void cleanup();

private:
    std::unordered_map<std::string, std::shared_ptr<PipelineContext>> pipelines_;
    mutable std::shared_mutex mtx_;
};
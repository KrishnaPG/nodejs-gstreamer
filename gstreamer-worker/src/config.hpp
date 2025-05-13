// config.hpp
#pragma once

#include <string>
#include <atomic>

extern const std::string ZMQ_PUB_ENDPOINT;
extern const std::string DEFAULT_LOG_LEVEL;
extern std::atomic<bool> enable_debug_logs;
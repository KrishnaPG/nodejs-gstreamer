// config.cpp
#include "config.hpp"

const std::string ZMQ_PUB_ENDPOINT = "tcp://*:5555";
const std::string DEFAULT_LOG_LEVEL = "info";
std::atomic<bool> enable_debug_logs = false;
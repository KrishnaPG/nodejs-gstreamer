#pragma once

#include <zmq.hpp>
#include <json-c/json.h>
#include <string>
#include <functional>
#include <thread>

class ZmqHandler {
public:
    using RpcCallback = std::function<void(json_object*)>;

    ZmqHandler(const std::string& endpoint, const std::string& topic, RpcCallback cb);
    ~ZmqHandler();

    void start();
    void publish_log(const std::string& level, const std::string& msg);

private:
    zmq::context_t context_;
    zmq::socket_t subscriber_;
    zmq::socket_t publisher_;
    std::string topic_;
    RpcCallback callback_;
    std::thread worker_thread_;
    bool running_ = true;

    void run();
};
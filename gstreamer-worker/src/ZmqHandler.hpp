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
    void publish_response(const std::string& topic, const std::string& payload);

private:
    zmq::context_t context_;
    zmq::socket_t subscriber_;
    zmq::socket_t publisher_;
    std::string topic_;
    RpcCallback callback_;

    static gboolean on_zmq_event(GSocket* socket, GIOCondition condition, gpointer user_data);
    void run();
};
#include "ZmqHandler.hpp"
#include <iostream>

ZmqHandler::ZmqHandler(const std::string& endpoint, const std::string& topic, RpcCallback cb)
    : subscriber_(context_, ZMQ_SUB),
      publisher_(context_, ZMQ_PUB),
      topic_(topic),
      callback_(std::move(cb)) {

    subscriber_.connect(endpoint);
    subscriber_.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.size());

    publisher_.bind("tcp://*:5555");
}

ZmqHandler::~ZmqHandler() {
    running_ = false;
    if (worker_thread_.joinable()) worker_thread_.join();
}

void ZmqHandler::start() {
    worker_thread_ = std::thread(&ZmqHandler::run, this);
}

void ZmqHandler::publish_log(const std::string& level, const std::string& msg) {
    json_object* log = json_object_new_object();
    json_object_object_add(log, "topic", json_object_new_string("log.pipeline"));
    json_object_object_add(log, "level", json_object_new_string(level.c_str()));
    json_object_object_add(log, "message", json_object_new_string(msg.c_str()));

    std::string payload = json_object_to_json_string_ext(log, JSON_C_TO_STRING_PLAIN);
    zmq::message_t topic_msg(topic_.data(), topic_.size());
    zmq::message_t data_msg(payload.data(), payload.size());

    publisher_.send(topic_msg, zmq::send_flags::sndmore);
    publisher_.send(data_msg, zmq::send_flags::none);

    json_object_put(log);
}

void ZmqHandler::run() {
    while (running_) {
        zmq::message_t topic_msg;
        if (!subscriber_.recv(&topic_msg, zmq::recv_flags::none)) continue;

        zmq::message_t data_msg;
        if (!subscriber_.recv(&data_msg, zmq::recv_flags::none)) continue;

        std::string method;
        json_object* root = json_tokener_parse(static_cast<const char*>(data_msg.data()));
        json_object* params = json_object_object_get(root, "params");

        if (callback_) callback_(params);

        json_object_put(root);
    }
}
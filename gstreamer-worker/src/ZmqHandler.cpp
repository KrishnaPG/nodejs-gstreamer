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

ZmqHandler::~ZmqHandler() = default;

void ZmqHandler::start() {
    GSocket* gsocket = g_socket_new_from_fd(zmq_getsockopt_ptr(subscriber_.get(), ZMQ_FD, NULL), NULL);
    GIOChannel* io_channel = g_io_channel_unix_new(g_socket_get_fd(gsocket));

    g_io_channel_set_flags(io_channel, G_IO_FLAG_NONBLOCK, nullptr);
    g_io_add_watch(io_channel, G_IO_IN, on_zmq_event, this, nullptr);

    g_object_unref(io_channel);
    g_object_unref(gsocket);
}

gboolean ZmqHandler::on_zmq_event(GSocket* socket, GIOCondition condition, gpointer user_data)
{
    auto handler = static_cast<ZmqHandler*>(user_data);

    zmq::message_t topic_msg, data_msg;
    if (!handler->subscriber_.recv(topic_msg, zmq::recv_flags::none)) return G_SOURCE_CONTINUE;
    if (!handler->subscriber_.recv(data_msg, zmq::recv_flags::none)) return G_SOURCE_CONTINUE;

    std::string json_str(static_cast<const char*>(data_msg.data()), data_msg.size());
    json_object* root = json_tokener_parse(json_str.c_str());

    if (root)
    {
        json_object* params = json_object_object_get(root, "params");
        if (params)
        {
            handler->callback_(params);
        }
        json_object_put(root);
    }

    return G_SOURCE_CONTINUE;
}

void ZmqHandler::publish_response(const std::string& topic, const std::string& payload)
{
    zmq::message_t topic_msg(topic.data(), topic.size());
    zmq::message_t data_msg(payload.data(), payload.size());

    publisher_.send(topic_msg, zmq::send_flags::sndmore);
    publisher_.send(data_msg, zmq::send_flags::none);
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
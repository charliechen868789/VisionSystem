#pragma once
#include "screen_event.pb.h"
#include <zmq.hpp>
#include <string>
#include <cstdint>

class VideoPublisher
{
public:
    VideoPublisher(const std::string &host, uint16_t port);
    ~VideoPublisher();
    void publish(const pfas::ScreenEvent &ev);

private:
    zmq::context_t m_ctx;
    zmq::socket_t  m_sock;
};
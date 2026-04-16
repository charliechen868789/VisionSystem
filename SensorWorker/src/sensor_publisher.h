#pragma once
#include "screen_event.pb.h"
#include <zmq.hpp>
#include <string>
#include <cstdint>

class SensorPublisher
{
public:
    SensorPublisher(const std::string &host, uint16_t port);
    ~SensorPublisher();
    void publishSensor(const std::string &id, float value,
                       const std::string &unit, const std::string &isoTime);
private:
    zmq::context_t m_ctx;
    zmq::socket_t  m_sock;
};
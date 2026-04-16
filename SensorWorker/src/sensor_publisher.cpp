#include "sensor_publisher.h"
#include <cstdio>
#include <chrono>
#include <ctime>

static uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

SensorPublisher::SensorPublisher(const std::string &host, uint16_t port)
    : m_ctx(1), m_sock(m_ctx, zmq::socket_type::pub)
{
    std::string ep = "tcp://" + host + ":" + std::to_string(port);
    m_sock.bind(ep);
    fprintf(stdout, "[SensorPublisher] bound to %s\n", ep.c_str());
}

SensorPublisher::~SensorPublisher() { m_sock.close(); m_ctx.close(); }

void SensorPublisher::publishSensor(const std::string &id, float value,
                                     const std::string &unit,
                                     const std::string &isoTime)
{
    pfas::ScreenEvent ev;
    ev.set_timestamp_ms(nowMs());
    ev.set_event_type(pfas::SENSOR_DATA);
    auto *s = ev.mutable_sensor_data();
    s->set_sensor_id(id);
    s->set_value(value);
    s->set_unit(unit);
    s->set_iso_time(isoTime);

    std::string bytes;
    if (!ev.SerializeToString(&bytes)) return;
    zmq::message_t msg(bytes.data(), bytes.size());
    m_sock.send(msg, zmq::send_flags::dontwait);
    fprintf(stdout, "[Sensor] %s = %.3f %s\n", id.c_str(), value, unit.c_str());
}
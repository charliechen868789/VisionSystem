#include "video_publisher.h"
#include <cstdio>

VideoPublisher::VideoPublisher(const std::string &host, uint16_t port)
    : m_ctx(1), m_sock(m_ctx, zmq::socket_type::pub)
{
    std::string ep = "tcp://" + host + ":" + std::to_string(port);
    m_sock.bind(ep);
    fprintf(stdout, "[VideoPublisher] bound to %s\n", ep.c_str());
}

VideoPublisher::~VideoPublisher() { m_sock.close(); m_ctx.close(); }

void VideoPublisher::publish(const pfas::ScreenEvent &ev)
{
    std::string bytes;
    if (!ev.SerializeToString(&bytes)) return;
    zmq::message_t msg(bytes.data(), bytes.size());
    m_sock.send(msg, zmq::send_flags::dontwait);
}
#pragma once
#include "screen_event.pb.h"
#include "hub_config.h"
#include <zmq.hpp>
#include <unordered_map>
#include <string>

class SettingsForwarder
{
public:
    explicit SettingsForwarder(const HubConfig &cfg);
    ~SettingsForwarder();

    void forward(const pfas::ControlActionPayload &ca);

private:
    zmq::context_t m_ctx;
    std::unordered_map<std::string, zmq::socket_t*> m_socks;

    // Maps action name → worker name
    static const std::unordered_map<std::string, std::string> k_actionWorker;
};
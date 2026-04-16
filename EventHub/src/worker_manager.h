#pragma once
#include "hub_config.h"
#include <string>
#include <unordered_map>
#include <unistd.h>

// Spawns each enabled worker as a child process.
// Monitors them and restarts on unexpected exit.
class WorkerManager
{
public:
    explicit WorkerManager(const HubConfig &cfg);
    ~WorkerManager();

    void startAll();
    void stopAll();

    // Call periodically from main loop — restarts dead workers
    void checkHealth();

private:
    struct Worker {
        std::string name;
        std::string binary;
        std::string config;
        pid_t       pid    = -1;
        bool        active = false;
    };

    pid_t spawnWorker(const Worker &w);

    const HubConfig              &m_cfg;
    std::unordered_map<std::string, Worker> m_workers;
};
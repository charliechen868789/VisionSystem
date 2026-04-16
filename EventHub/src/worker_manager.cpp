#include "worker_manager.h"
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <sys/wait.h>

WorkerManager::WorkerManager(const HubConfig &cfg) : m_cfg(cfg)
{
    for (const auto &[name, entry] : cfg.workers) {
        if (!entry.enabled) continue;
        Worker w;
        w.name   = name;
        w.binary = entry.binary;
        w.config = entry.config;
        m_workers[name] = w;
    }
}

WorkerManager::~WorkerManager() { stopAll(); }

void WorkerManager::startAll()
{
    for (auto &[name, w] : m_workers) {
        w.pid    = spawnWorker(w);
        w.active = (w.pid > 0);
    }
}

void WorkerManager::stopAll()
{
    for (auto &[name, w] : m_workers) {
        if (w.pid > 0) {
            fprintf(stdout, "[WorkerManager] stopping %s (pid %d)\n",
                    name.c_str(), w.pid);
            kill(w.pid, SIGTERM);
            waitpid(w.pid, nullptr, 0);
            w.pid    = -1;
            w.active = false;
        }
    }
}

void WorkerManager::checkHealth()
{
    for (auto &[name, w] : m_workers) {
        if (!w.active) continue;
        int status = 0;
        pid_t ret  = waitpid(w.pid, &status, WNOHANG);
        if (ret == w.pid) {
            fprintf(stderr, "[WorkerManager] %s (pid %d) exited — restarting\n",
                    name.c_str(), w.pid);
            w.pid    = spawnWorker(w);
            w.active = (w.pid > 0);
        }
    }
}

pid_t WorkerManager::spawnWorker(const Worker &w)
{
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "[WorkerManager] fork failed for %s: %s\n",
                w.name.c_str(), strerror(errno));
        return -1;
    }
    if (pid == 0) {
        // Child
        const char *args[] = {
            w.binary.c_str(),
            "--config", w.config.c_str(),
            nullptr
        };
        execv(w.binary.c_str(), const_cast<char *const *>(args));
        fprintf(stderr, "[WorkerManager] execv failed for %s: %s\n",
                w.binary.c_str(), strerror(errno));
        _exit(1);
    }
    fprintf(stdout, "[WorkerManager] started %s (pid %d)\n", w.name.c_str(), pid);
    return pid;
}
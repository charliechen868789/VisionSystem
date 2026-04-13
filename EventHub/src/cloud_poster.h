#pragma once
#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

struct PostJob {
    std::string eventType;
    std::string jsonBody;   // serialised for the REST API
    int retries = 0;
};

class CloudPoster
{
public:
    struct Config {
        std::string endpoint = "http://localhost:8080/api/events";
        std::string apiKey;
        int         maxRetries    = 3;
        long        timeoutSec    = 10;
    };

    explicit CloudPoster(Config cfg);
    ~CloudPoster();

    void post(const std::string &eventType, const std::string &jsonBody);
    void stop();

private:
    void workerLoop();
    bool doPost(const PostJob &job);

    Config     m_cfg;
    std::queue<PostJob>        m_queue;
    std::mutex                 m_mu;
    std::condition_variable    m_cv;
    std::atomic<bool>          m_running{true};
    std::thread                m_worker;
};
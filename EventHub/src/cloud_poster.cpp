#include "cloud_poster.h"
#include <curl/curl.h>
#include <cstdio>
#include <chrono>
#include <thread>

CloudPoster::CloudPoster(Config cfg) : m_cfg(std::move(cfg))
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    m_worker = std::thread([this]{ workerLoop(); });
}

CloudPoster::~CloudPoster() { stop(); curl_global_cleanup(); }

void CloudPoster::post(const std::string &type, const std::string &body)
{
    std::lock_guard<std::mutex> lk(m_mu);
    m_queue.push({type, body, 0});
    m_cv.notify_one();
}

void CloudPoster::stop()
{
    m_running = false;
    m_cv.notify_all();
    if (m_worker.joinable()) m_worker.join();
}

void CloudPoster::workerLoop()
{
    while (m_running) {
        PostJob job;
        {
            std::unique_lock<std::mutex> lk(m_mu);
            m_cv.wait(lk, [this]{ return !m_queue.empty() || !m_running; });
            if (!m_running && m_queue.empty()) break;
            job = m_queue.front();
            m_queue.pop();
        }

        bool ok = doPost(job);
        if (!ok && job.retries < m_cfg.maxRetries) {
            ++job.retries;
            int delay = (1 << job.retries);   // exponential back-off seconds
            fprintf(stderr, "[CloudPoster] retry %d in %ds\n", job.retries, delay);
            std::this_thread::sleep_for(std::chrono::seconds(delay));
            std::lock_guard<std::mutex> lk(m_mu);
            m_queue.push(job);
            m_cv.notify_one();
        }
    }
}

bool CloudPoster::doPost(const PostJob &job)
{
    CURL *curl = curl_easy_init();
    if (!curl) return false;

    curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!m_cfg.apiKey.empty()) {
        std::string auth = "Authorization: Bearer " + m_cfg.apiKey;
        headers = curl_slist_append(headers, auth.c_str());
    }

    long httpCode = 0;
    curl_easy_setopt(curl, CURLOPT_URL,            m_cfg.endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     job.jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  (long)job.jsonBody.size());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        m_cfg.timeoutSec);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  // suppress stdout output
        +[](char*,size_t s,size_t n,void*)->size_t{ return s*n; });

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    bool ok = (res == CURLE_OK && httpCode >= 200 && httpCode < 300);
    fprintf(stdout, "[CloudPoster] POST %s  http=%ld  %s\n",
            job.eventType.c_str(), httpCode, ok ? "OK" : "FAIL");
    return ok;
}
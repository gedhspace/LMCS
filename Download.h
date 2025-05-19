#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <curl/curl.h>
#include <algorithm>
#include <cstdio>
#include <memory>


#pragma warning(disable : 4996)

class DownloadManager {
public:

    void SetProxy(const std::string& proxy) {
        proxy_ = proxy;
    }

    explicit DownloadManager(int num_threads = 4)
        : running_(true), progress_running_(true) {
        curl_global_init(CURL_GLOBAL_ALL);
        for (int i = 0; i < num_threads; ++i) {
            workers_.emplace_back(&DownloadManager::WorkerThread, this);
        }
        progress_thread_ = std::thread(&DownloadManager::ProgressDisplayThread, this);
    }

    ~DownloadManager() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            running_ = false;
            progress_running_ = false;
        }
        queue_cv_.notify_all();
        for (auto& thread : workers_) {
            if (thread.joinable()) thread.join();
        }
        if (progress_thread_.joinable()) progress_thread_.join();
        curl_global_cleanup();
    }

    void AddDownload(const std::string& url, const std::string& filename) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        download_queue_.emplace(url, filename);
        queue_cv_.notify_one();
    }

    void WaitForCompletion() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        completion_cv_.wait(lock, [this]() {
            return download_queue_.empty() && active_threads_ == 0;
            });
    }

private:


    std::string proxy_; // 新增成员变量

    struct DownloadProgress {
        std::string filename;
        std::string url;
        curl_off_t total_bytes = 0;
        curl_off_t downloaded_bytes = 0;
        double speed = 0.0;
        bool completed = false;
        std::chrono::time_point<std::chrono::system_clock> start_time;
        std::chrono::time_point<std::chrono::system_clock> last_update;
        curl_off_t last_bytes = 0;
    };

    void WorkerThread() {
        while (running_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]() {
                return !download_queue_.empty() || !running_;
                });

            if (!running_) break;

            auto task = download_queue_.front();
            download_queue_.pop();
            ++active_threads_;
            lock.unlock();

            auto progress = std::make_shared<DownloadProgress>();
            progress->filename = task.second;
            progress->url = task.first;
            progress->start_time = std::chrono::system_clock::now();
            progress->last_update = progress->start_time;
            progress->last_bytes = 0;

            {
                std::lock_guard<std::mutex> guard(progress_mutex_);
                active_downloads_.push_back(progress);
            }

            CURL* curl = curl_easy_init();
            //FILE* fp = fopen(task.second.c_str(), "wb");

            filesystem::path save_path(task.second);
            filesystem::create_directories(save_path.parent_path()); // 新增目录创建
            FILE* fp = fopen(save_path.string().c_str(), "wb");


            if (curl && fp) {
                curl_easy_setopt(curl, CURLOPT_URL, task.first.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
                curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progress.get());
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
                curl_easy_setopt(curl, CURLOPT_HEADERDATA, progress.get());

                if (!proxy_.empty()) {
                    curl_easy_setopt(curl, CURLOPT_PROXY, proxy_.c_str());
                    curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
                }

                CURLcode res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    std::cerr << "\nDownload failed: "
                        << curl_easy_strerror(res) << std::endl;
                }

                fclose(fp);
                curl_easy_cleanup(curl);
            }
            else {
                std::cerr << "Failed to initialize CURL or open file" << std::endl;
            }

            {
                std::lock_guard<std::mutex> guard(progress_mutex_);
                progress->completed = true;
                auto it = std::find(active_downloads_.begin(),
                    active_downloads_.end(), progress);
                if (it != active_downloads_.end()) {
                    active_downloads_.erase(it);
                }
            }

            lock.lock();
            --active_threads_;
            if (download_queue_.empty() && active_threads_ == 0) {
                completion_cv_.notify_all();
            }
        }
    }

    static int ProgressCallback(void* clientp,
        curl_off_t dltotal,
        curl_off_t dlnow,
        curl_off_t ultotal,
        curl_off_t ulnow) {
        auto* progress = static_cast<DownloadProgress*>(clientp);
        if (!progress) return 0;

        auto now = std::chrono::system_clock::now();
        auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - progress->last_update);

        if (time_span.count() > 500) {
            double time_diff = time_span.count() / 1000.0;
            curl_off_t bytes_diff = dlnow - progress->last_bytes;

            if (time_diff > 0) {
                progress->speed = bytes_diff / time_diff;
            }

            progress->last_bytes = dlnow;
            progress->last_update = now;
        }

        progress->downloaded_bytes = dlnow;
        if (dltotal > 0) {
            progress->total_bytes = dltotal;
        }

        return 0;
    }

    static size_t HeaderCallback(char* buffer, size_t size,
        size_t nitems, void* userdata) {
        auto* progress = static_cast<DownloadProgress*>(userdata);
        const std::string header(buffer, size * nitems);

        if (header.find("Content-Length:") != std::string::npos) {
            std::string length_str = header.substr(header.find(":") + 2);
            length_str.erase(std::remove(length_str.begin(), length_str.end(), '\r'), length_str.end());
            length_str.erase(std::remove(length_str.begin(), length_str.end(), '\n'), length_str.end());
            try {
                progress->total_bytes = std::stoull(length_str);
            }
            catch (const std::exception& e) {
                std::cerr << "Error parsing Content-Length: " << e.what() << std::endl;
            }
        }

        return size * nitems;
    }

    void ProgressDisplayThread() {
        while (progress_running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            std::vector<std::shared_ptr<DownloadProgress>> current_downloads;
            {
                std::lock_guard<std::mutex> guard(progress_mutex_);
                current_downloads = active_downloads_;
            }

            std::system("cls");
            std::cout << "\033[32mActive Downloads:\033[0m\n";

            double total_speed = 0.0;
            for (const auto& progress : current_downloads) {
                total_speed += progress->speed;

                double percent = 0.0;
                if (progress->total_bytes > 0) {
                    percent = (static_cast<double>(progress->downloaded_bytes) /
                        progress->total_bytes) * 100.0;
                }

                auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now() - progress->start_time);

                std::cout << "[\033[33m" << std::left << std::setw(20)
                    << progress->filename << "\033[0m] "
                    << GenerateProgressBar(percent) << " "
                    << std::fixed << std::setprecision(1) << percent << "% "
                    << "(\033[36m" << FormatSpeed(progress->speed)
                    << "\033[0m) - " << duration.count() << "s\n";
            }

            std::cout << "\n\033[32mTotal Speed: \033[35m"
                << FormatSpeed(total_speed) << "\033[0m\n"
                << "\033[32mActive Threads: \033[33m"
                << active_threads_ << "\033[0m\n";
        }
    }

    static std::string FormatSpeed(double speed) {
        const char* units[] = { "B/s", "KB/s", "MB/s", "GB/s" };
        int unit = 0;
        while (speed > 1024 && unit < 3) {
            speed /= 1024;
            unit++;
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << speed << " " << units[unit];
        return ss.str();
    }

    static std::string GenerateProgressBar(double percent, int width = 30) {
        int filled = static_cast<int>(percent * width / 100.0);
        std::string bar = "\033[34m[\033[0m";
        for (int i = 0; i < width; ++i) {
            bar += (i < filled) ? "\033[32m=\033[0m" : " ";
        }
        bar += "\033[34m]\033[0m";
        return bar;
    }

    static size_t WriteCallback(void* contents, size_t size,
        size_t nmemb, void* userp) {
        return fwrite(contents, size, nmemb, static_cast<FILE*>(userp));
    }

    std::queue<std::pair<std::string, std::string>> download_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::condition_variable completion_cv_;
    std::vector<std::thread> workers_;
    std::atomic<int> active_threads_{ 0 };
    bool running_;

    std::vector<std::shared_ptr<DownloadProgress>> active_downloads_;
    std::mutex progress_mutex_;
    std::thread progress_thread_;
    std::atomic<bool> progress_running_;
};

#endif // DOWNLOAD_H
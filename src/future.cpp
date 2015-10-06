#include "future.h"
#include <queue>
#include <vector>
#include <atomic>
#include <thread>

class threadpool_executor : public executor {
public:
  threadpool_executor() {
    for (size_t n = 0; n < std::max(std::thread::hardware_concurrency() * 2, 4u); ++n) {
      threads.emplace_back([this] {this->worker();});
    }
  }
  ~threadpool_executor() {
    while (!tasks.empty()) {
      std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
    shutdown.store(true);
    for (auto &t : threads) {
      t.join();
    }
  }
  void queue(std::function<void()> func) override {
    std::lock_guard<std::mutex> l(m);
    tasks.push(func);
  }
  std::queue<std::function<void()>> tasks;
  std::vector<std::thread> threads;
  std::mutex m;
  std::atomic<bool> shutdown;
  void worker() {
    while (!shutdown.load()) {
      bool foundTask = false;
      std::function<void()> f;
      {
        std::lock_guard<std::mutex> l(m);
        if (!tasks.empty()) {
          f = tasks.front();
          tasks.pop();
          foundTask = true;
        }
      }

      if (foundTask) {
        f();
      } else {
        std::this_thread::sleep_for(std::chrono::microseconds(500));
      }
    }
  }
};

executor *default_executor() {
  static threadpool_executor exec;
  return &exec;
}



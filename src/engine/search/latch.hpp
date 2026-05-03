#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace episteme::latch {

class Latch {
  public:
  void arm(int num) {
    std::lock_guard<std::mutex> lock(mutex);
    count = num;
  }

  void done() {
    {
      std::lock_guard<std::mutex> lock(mutex);
      --count;
      cond.notify_all();
    }
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock, [this] { return count == 0; });
  }

  void wait_for(int target) {
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock, [this, target] { return count <= target; });
  }

  private:
  std::mutex mutex;
  std::condition_variable cond;
  size_t count = 0;
};
}  // namespace episteme::latch

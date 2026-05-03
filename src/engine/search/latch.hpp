#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace episteme::latch {

class Latch {
  public:
  void arm(int num) {
    std::lock_guard lock(mutex);
    count = num;
  }

  void done() {
    bool last;
    {
      std::lock_guard lock(mutex);
      last = (--count == 0);
    }
    if (last) cond.notify_all();
  }

  void wait() {
    std::unique_lock lock(mutex);
    cond.wait(lock, [this] { return count == 0; });
  }

  private:
  std::mutex mutex;
  std::condition_variable cond;
  size_t count = 0;
};
}  // namespace episteme::latch

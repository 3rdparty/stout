#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

////////////////////////////////////////////////////////////////////////

namespace stout {

////////////////////////////////////////////////////////////////////////

template <typename T>
class Notification {
 public:
  Notification()
    : notified_(false) {}

  void Notify(T t) {
    // Need to move functions to local so that we can invoke
    // them outside of mutex in the event that invoking them
    // deletes this instance and thus the mutex.
    std::vector<std::function<void(T)>> functions;

    mutex_.lock();

    // Copy 't' rather than 'std::move' so that we can use 't' when
    // invoking the functions in case one of the callbacks deletes
    // this instance.
    t_ = t;

    notified_.store(true, std::memory_order_release);

    condition_.notify_all();

    functions = std::move(functions_);

    mutex_.unlock();

    // NOTE: explicit design goal to execute handlers in reverse order
    // they were added. This works similar to how destructors are
    // called on the stack in reverse order to get constructed.
    while (!functions.empty()) {
      // See comment above for why we use 't' instead of 't_'.
      functions.back()(t);
      functions.pop_back();
    }
  }

  void Watch(std::function<void(T)>&& f) {
    if (notified_.load(std::memory_order_acquire)) {
      f(t_);
    } else {
      mutex_.lock();
      if (notified_.load(std::memory_order_acquire)) {
        mutex_.unlock();
        f(t_);
      } else {
        functions_.push_back(std::move(f));
        mutex_.unlock();
      }
    }
  }

  T Wait() {
    if (!notified_.load(std::memory_order_acquire)) {
      std::unique_lock<std::mutex> lock(mutex_);
      while (!notified_.load(std::memory_order_acquire)) {
        condition_.wait(lock);
      }
    }
    return t_;
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  T t_;
  std::once_flag notify_;
  std::atomic<bool> notified_;
  std::vector<std::function<void(T)>> functions_;
};

////////////////////////////////////////////////////////////////////////

} // namespace stout

////////////////////////////////////////////////////////////////////////

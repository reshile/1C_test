#pragma once

#include "headers.hpp"
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
 protected:
  std::mutex mutex;
  std::deque<T> queue;

  std::condition_variable block;
  std::mutex cv_mutex;

 public:

  ThreadSafeQueue() = default;
  ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;

  const T& front() {
    std::scoped_lock lock(mutex);
    return queue.front();
  }

  const T& back() {
    std::scoped_lock lock(mutex);
    return queue.back();
  }
   
  void push_back(const T& obj) {
    std::scoped_lock lock(mutex);
    queue.push_back(std::move(obj));

    std::unique_lock<std::mutex> ulock(cv_mutex);
    block.notify_one();
  }

  void push_front(const T& obj) {
    std::scoped_lock lock(mutex);
    queue.push_front(std::move(obj));

    std::unique_lock<std::mutex> ulock(cv_mutex);
    block.notify_one();
  }

  bool empty() {
    std::scoped_lock lock(mutex);
    return queue.empty();
  }

  size_t size() {
    std::scoped_lock lock(mutex);
    return queue.size();
  }

  void clear() {
    std::scoped_lock lock(mutex);
    queue.clear();
  }

  T pop_front() {
    std::scoped_lock lock(mutex);
    T copy = std::move(queue.front());
    queue.pop_front();
    return copy;
  }

  T pop_back() {
    std::scoped_lock lock(mutex);
    T copy = std::move(queue.back());
    queue.pop_back();
    return copy;
  }

  void wait() {
    while (empty()) {
      std::unique_lock<std::mutex> lock(cv_mutex);
      block.wait(lock);
    } 
  }
};

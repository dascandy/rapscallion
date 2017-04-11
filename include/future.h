#ifndef FUTURE_H
#define FUTURE_H

#include <mutex>
#include <condition_variable>
#include <memory>
#include <exception>
#include <type_traits>

struct promise_exception : public std::exception {
};

struct promise_already_satisfied : promise_exception {};

template <typename T>
class result {
public:
  ~result()
  {
    if (value_set)
      reinterpret_cast<T*>(&storage)->~T();
  }
  void set(T &&value) {
    std::unique_lock<std::mutex> lk(m);
    if (value_set)
      throw promise_already_satisfied();
    new (&storage) T(std::move(value));
    value_set = true;
    cv.notify_all();
  }
  void set_error(std::exception_ptr eptr) {
    std::unique_lock<std::mutex> lk(m);
    if (value_set)
      throw promise_already_satisfied();
    this->eptr = eptr;
    value_set = true;
    cv.notify_all();
  }
  const T& get() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [this]{ return value_set; });
    if (eptr)
      std::rethrow_exception(eptr);
    return *reinterpret_cast<T*>(&storage);
  }
  typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type storage;
  std::exception_ptr eptr;
  mutable std::mutex m;
  mutable std::condition_variable cv;
  bool value_set = false;
};

template <typename T>
class handle {
public:
  const T& get() const {
    return state->get();
  }
private:
  handle(std::shared_ptr<result<T>> state) : state(state) {}
  std::shared_ptr<result<T>> state;
};

#endif



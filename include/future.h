#ifndef FUTURE_H
#define FUTURE_H

#include <future>
#include <memory>
#include <type_traits>

template <typename T>
class result {
public:
  result(result&& rhs) = delete;
  result& operator=(result&& rhs) = delete;
  ~result()
  {
    if (promise_made) {
      promise_ref().~promise<T>();
    }
  }
  std::future<T> get_local() {
    if (!promise_made) {
      new (&storage) std::promise<T>();
      promise_made = true;
      // TODO: signal to the remote that we actually wish to receive the value associated with this.
    }

    return promise_ref().get_future();
  }

private:
  std::promise<T>& promise_ref()
  {
    return *reinterpret_cast<std::promise<T>*>(&storage);
  }

private:
  typename std::aligned_storage<sizeof(std::promise<T>), std::alignment_of<std::promise<T>>::value>::type storage;
  bool promise_made = false;
};

template <typename T>
class handle {
public:
  std::future<T> get_local() {
    return state->get_local();
  }
private:
  handle(std::shared_ptr<result<T>> state) : state(std::move(state)) {}
  std::shared_ptr<result<T>> state;
};

#endif



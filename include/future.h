#ifndef FUTURE_H
#define FUTURE_H

#include <boost/optional.hpp>
#include <future>
#include <memory>

template <typename T>
class result {
public:
  result(result&& rhs) = delete;
  result& operator=(result&& rhs) = delete;

  std::future<T> get_local() {
    if (!promise) {
      promise = std::promise<T>();
      // TODO: signal to the remote that we actually wish to receive the value associated with this.
    }

    return promise->get_future();
  }

private:
  boost::optional<std::promise<T>> promise;
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



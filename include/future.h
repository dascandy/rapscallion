#ifndef FUTURE_H
#define FUTURE_H

#include <boost/optional.hpp>
#include <future>
#include <memory>
#include "serializer.h"

struct Serializer;

// TODO: add an actual implementation of this thing
template <typename T, typename E>
struct expected;

template <typename T>
struct handle_promise {
  typedef std::promise<T> promise_type;

  void operator()(promise_type& value, Serializer& s) const {
      value.set_value(reader<T>::read(s));
  }
};

template <>
struct handle_promise<void> {
  typedef std::promise<void> promise_type;
  void operator()(promise_type& value, Serializer& ) const {
    value.set_value();
  }
};

template <typename T, typename E>
struct handle_promise<expected<T, E>> {
  typedef std::promise<T> promise_type;
  void operator()(promise_type& value, Serializer& s) const {
    auto e = reader<expected<T, E>>::read(s);
    if (e)
      value.set_value(*e);
    else
      value.set_error(e.error());
  }
};

template <typename E>
struct handle_promise<expected<void, E>> {
  typedef std::promise<void> promise_type;
  void operator()(promise_type& value, Serializer& s) const {
    auto e = reader<expected<void, E>>::read(s);
    if (e)
      value.set_value();
    else
      value.set_exception(e.error());
  }
};

struct result_base
{
  virtual void deserialize_from(Serializer&) = 0;
};

template <typename T>
class result : public result_base {
public:

  result(result&& rhs) = delete;
  result& operator=(result&& rhs) = delete;

  std::future<T> get_local() {
    if (!promise) {
      promise = promise_type();
      // TODO: signal to the remote that we actually wish to receive the value associated with this.
    }

    return promise->get_future();
  }

  void deserialize_from(Serializer& deserializer) override
  {
    handle_promise<T>()(*promise, deserializer);
  }

private:
  typedef typename handle_promise<T>::promise_type promise_type;
  boost::optional<promise_type> promise;
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



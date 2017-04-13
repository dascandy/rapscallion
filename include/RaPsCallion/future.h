#ifndef FUTURE_H
#define FUTURE_H

#include <boost/optional.hpp>
#include <future>
#include <memory>
#include "serializer.h"

namespace rapscallion {

// TODO: add an actual implementation of this thing
template <typename... E>
struct remote_exception_ptr;

// TODO: add an actual implementation of this thing
template <typename T, typename E>
struct expected;

template <typename T>
struct handle_promise {
  typedef std::promise<T> promise_type;

  void operator()(promise_type& value, Deserializer& s) const {
      value.set_value(serializer<T>::read(s));
  }
};

template <>
struct handle_promise<void> {
  typedef std::promise<void> promise_type;
  void operator()(promise_type& value, Deserializer& ) const {
    value.set_value();
  }
};

template <typename T, typename... E>
struct handle_promise<expected<T, remote_exception_ptr<E...>>> {
  typedef std::promise<T> promise_type;
  void operator()(promise_type& value, Deserializer& s) const {
    auto e = serializer<expected<T, remote_exception_ptr<E...>>>::read(s);
    if (e)
      value.set_value(*e);
    else
      value.set_error(e.error().make_exception_ptr());
  }
};

template <typename... E>
struct handle_promise<expected<void, remote_exception_ptr<E...>>> {
  typedef std::promise<void> promise_type;
  void operator()(promise_type& value, Deserializer& s) const {
    auto e = serializer<expected<void, remote_exception_ptr<E...>>>::read(s);
    if (e)
      value.set_value();
    else
      value.set_exception(e.error().make_exception_ptr());
  }
};

struct result_base
{
  virtual ~result_base() = default;
  virtual void deserialize_from(Deserializer&) = 0;
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

  void deserialize_from(Deserializer& deserializer) override
  {
    handle_promise<T>()(*promise, deserializer);
  }

private:
  typedef typename handle_promise<T>::promise_type promise_type;
  boost::optional<promise_type> promise;
};

template <typename T>
class future {
public:
  future(const T&);
  future(T&&);

  std::future<T> get_local() {
    return state->get_local();
  }
private:
  future(std::shared_ptr<result<T>> state) : state(std::move(state)) {}
  std::shared_ptr<result<T>> state;
};

}

#endif

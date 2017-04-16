#ifndef FUTURE_H
#define FUTURE_H

#include <boost/optional.hpp>
#include <future>
#include <memory>
#include <utility>
#include "serializer.h"

namespace rapscallion {

namespace detail {
  template <typename T>
  struct handle_promise {
    void operator()(std::promise<T>& value, Deserializer& s) const {
        value.set_value(serializer<T>::read(s));
    }
  };

  template <>
  struct handle_promise<void> {
    void operator()(std::promise<void>& value, Deserializer& ) const {
      value.set_value();
    }
  };
}

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
      promise = std::promise<T>();
      // TODO: signal to the remote that we actually wish to receive the value associated with this.
    }

    return promise->get_future();
  }

  void deserialize_from(Deserializer& deserializer) override
  {
    handle_promise<T>()(*promise, deserializer);
  }

private:
  boost::optional<std::promise<T>> promise;
};

template <typename T>
class future {
public:
  std::future<T> get_local() {
    return state->get_local();
  }
private:
  future(std::shared_ptr<result<T>> state) : state(std::move(state)) {}
  std::shared_ptr<result<T>> state;
};

}

#endif

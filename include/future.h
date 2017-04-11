#ifndef FUTURE_H
#define FUTURE_H

#include <future>
#include <memory>
#include <type_traits>

template <typename T>
class result {
public:
  result(result&& rhs)
    : promise_made(rhs.promise_made)
  {
    if (rhs.promise_made) {
      new (&storage) std::promise<T>(std::move(*reinterpret_cast<std::promise<T>*>(&rhs.storage)));
    }
  }

  result& operator=(result&& rhs)
  {
    if (rhs.promise_made)
    {
      if (promise_made)
      {
        *reinterpret_cast<std::promise<T>*>(&storage) = std::move(*reinterpret_cast<std::promise<T>*>(&rhs.storage));
      }
      else
      {
        new (&storage) std::promise<T>(std::move(*reinterpret_cast<std::promise<T>*>(&rhs.storage)));
        promise_made = true;
      }
    }
    else
    {
      destroy();
    }

    return *this;
  }

  ~result()
  {
    destroy();
  }

  std::future<T> get_local() {
    if (!promise_made) {
      new (&storage) std::promise<T>();
      promise_made = true;
      // TODO: signal to the remote that we actually wish to receive the value associated with this.
    }

    return reinterpret_cast<std::promise<T>*>(&storage)->get_future();
  }

private:
  void destroy()
  {
    if (promise_made) {
      reinterpret_cast<std::promise<T>*>(&storage)->~promise<T>();
      promise_made = false;
    }
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



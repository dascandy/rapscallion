#pragma once

#include <functional>
#include <thread>
#include <map>
#include <vector>
#include <typeinfo>
#include <atomic>
#include <exception>
#include <mutex>
#include "future.h"
#include "Serializer.h"
#include "RpcClient.h"

namespace Rapscallion {

struct Serializer;
class IProxy;
class IHasDispatch;
class RpcClient;

class IProxy : public std::enable_shared_from_this<IProxy> {
public:
  virtual void signalDisconnect() = 0;
  virtual void dispatch(Deserializer& s) = 0;
  virtual ~IProxy() {}
};

template <typename I>
class ProxyBase : public I, public IProxy {
public:
  template <typename T>
  static constexpr std::string interfaceNameOf() {
    const char *str = typeid(T).name();
    while (*str && *str <= '9') ++str;
    return str;
  }
  std::string interfaceName = interfaceNameOf<I>();
  std::string getInterfaceName() { 
    return interfaceName;
  }
  ProxyBase(std::shared_ptr<RpcClient> conn)
  : conn(conn)
  {
  }
  void signalDisconnect() {
    for (const auto &p : callbacks) {
      Deserializer s;
      p.second(s);
    }
    callbacks.clear();
    conn = NULL;
  }
protected:
  size_t getRequestId() {
    std::unique_lock<std::mutex> lock(cbM);
    return requestId++;
  }
  template <typename T> future<T> getFutureFor(size_t requestId) {
    std::unique_lock<std::mutex> lock(cbM);
    std::shared_ptr<promise<T>> value = std::make_shared<promise<T>>();
    future<T> rv = value->get_future();
    callbacks[requestId] = [value](Deserializer&s){
      try {
        value->set_value(serializer<T>::read(s));
      } catch (...) {
        value->set_exception(std::current_exception());
      }
    };
    return rv;
  }
public:
  void dispatch(Deserializer& s) override {
    std::function<void(Deserializer&)> cb;
    {
      std::unique_lock<std::mutex> lock(cbM);
      size_t id = serializer<size_t>::read(s);
      auto it = callbacks.find(id);
      if (it == callbacks.end()) {
        printf("No callback found for requestID %zu on %p/%p interface %s\n", id, (void*)this, (void*)conn.get(), getInterfaceName().c_str());
        return;
      }
      cb = it->second;
      callbacks.erase(it);
    }
    cb(s);
  }
  std::shared_ptr<RpcClient> conn;
private:
  size_t requestId = 0;
  std::map<size_t, std::function<void(Deserializer&)>> callbacks;
  std::mutex cbM;
};

#define PROXY_PROLOG(name, type)  \
  size_t reqId = getRequestId(); \
  future<type> f = getFutureFor<type>(reqId); \
  Rapscallion::Serializer s; \
  Rapscallion::serializer<std::string>::write(s, getInterfaceName()); \
  Rapscallion::serializer<std::string>::write(s, #name); \
  Rapscallion::serializer<size_t>::write(s, reqId);
#define PROXY_EPILOG(type) \
  conn->Send(s); \
  return f;
#define PROXY_FUNC0(name, type) future<type> name() override { PROXY_PROLOG(name, type) PROXY_EPILOG(type) }
#define PROXY_FUNC1(name, type, a1) future<type> name(a1 A1) override { PROXY_PROLOG(name, type) Rapscallion::serializer<a1>::write(s, A1); PROXY_EPILOG(type) }
#define PROXY_FUNC2(name, type, a1, a2) future<type> name(a1 A1, a2 A2) override { PROXY_PROLOG(name, type) Rapscallion::serializer<a1>::write(s, A1); Rapscallion::serializer<a2>::write(s, A2); PROXY_EPILOG(type) }
#define PROXY_FUNC3(name, type, a1, a2, a3) future<type> name(a1 A1, a2 A2, a3 A3) override { PROXY_PROLOG(name, type) Rapscallion::serializer<a1>::write(s, A1); Rapscallion::serializer<a2>::write(s, A2); Rapscallion::serializer<a3>::write(s, A3); PROXY_EPILOG(type) }
#define PROXY_FUNC4(name, type, a1, a2, a3, a4) future<type> name(a1 A1, a2 A2, a3 A3, a4 A4) override { PROXY_PROLOG(name, type) Rapscallion::serializer<a1>::write(s, A1); Rapscallion::serializer<a2>::write(s, A2); Rapscallion::serializer<a3>::write(s, A3); Rapscallion::serializer<a4>::write(s, A4); PROXY_EPILOG(type) }
#define PROXY_FUNC5(name, type, a1, a2, a3, a4, a5) future<type> name(a1 A1, a2 A2, a3 A3, a4 A4, a5 A5) override { PROXY_PROLOG(name, type) Rapscallion::serializer<a1>::write(s, A1); Rapscallion::serializer<a2>::write(s, A2); Rapscallion::serializer<a3>::write(s, A3); Rapscallion::serializer<a4>::write(s, A4); Rapscallion::serializer<a5>::write(s, A5); PROXY_EPILOG(type) }
#define PROXY_FUNC6(name, type, a1, a2, a3, a4, a5, a6) future<type> name(a1 A1, a2 A2, a3 A3, a4 A4, a5 A5, a6 A6) override { PROXY_PROLOG(name, type) Rapscallion::serializer<a1>::write(s, A1); Rapscallion::serializer<a2>::write(s, A2); Rapscallion::serializer<a3>::write(s, A3); Rapscallion::serializer<a4>::write(s, A4); Rapscallion::serializer<a5>::write(s, A5); Rapscallion::serializer<a6>::write(s, A6); PROXY_EPILOG(type) }
#define PROXY_FUNC7(name, type, a1, a2, a3, a4, a5, a6, a7) future<type> name(a1 A1, a2 A2, a3 A3, a4 A4, a5 A5, a6 A6, a7 A7) override { PROXY_PROLOG(name, type) Rapscallion::serializer<a1>::write(s, A1); Rapscallion::serializer<a2>::write(s, A2); Rapscallion::serializer<a3>::write(s, A3); Rapscallion::serializer<a4>::write(s, A4); Rapscallion::serializer<a5>::write(s, A5); Rapscallion::serializer<a6>::write(s, A6); Rapscallion::serializer<a7>::write(s, A7); PROXY_EPILOG(type) }

}


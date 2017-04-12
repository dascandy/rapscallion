#ifndef RPC_H
#define RPC_H

#include <functional>
#include <thread>
#include <map>
#include <vector>
#include <typeinfo>
#include <atomic>
#include <exception>
#include "serializer.h"
#include "future.h"
#include "connection.h"

class remote_exception : public std::runtime_error {
public:
  remote_exception(const std::string& value) 
  : std::runtime_error(value)
  {}
};

template <typename T>
std::string interfaceNameOf() {
  const char *str = typeid(T).name();
  while (*str <= '9') ++str;
  return str;
}

struct Serializer;
class IProxy;
class IHasDispatch;
class Connection;

class IProxy : public std::enable_shared_from_this<IProxy> {
public:
  virtual void signalDisconnect() = 0;
  virtual void dispatch(Serializer& s) = 0;
  virtual ~IProxy() {}
};

template <typename I>
class ProxyBase : public I, public IProxy {
public:
  std::string interfaceName;
  std::string getInterfaceName() { 
    return interfaceName;
  }
  ProxyBase(std::shared_ptr<Connection> conn)
  : interfaceName(interfaceNameOf<I>())
  , conn(conn)
  {
  }
  void signalDisconnect() {
    for (const auto &p : callbacks) {
      Serializer s;
      p.second(s);
    }
    callbacks.clear();
    conn = NULL;
  }
  static std::shared_ptr<ProxyBase<I>> Create(std::unique_lock<std::mutex> &lock, std::shared_ptr<Connection> c);
protected:
  size_t getRequestId() {
    std::unique_lock<std::mutex> lock(cbM);
    return requestId++;
  }
  template <typename T> future<T> getFutureFor(size_t requestId) {
    std::unique_lock<std::mutex> lock(cbM);
    std::shared_ptr<promise<T>> value = std::make_shared<promise<T>>();
    future<T> rv = value->get_future();
    callbacks[requestId] = [value](Serializer&s){
      if (s.buffer->size() == 0) {
        try {
          throw remote_exception("Connection Lost");
        } catch (...) {
          value->set_error(std::current_exception());
        }
      } else {
        handle_promise<T>::handle(value, s);
      // Any exception propagation can be added here
      }
    };
    return std::move(rv);
  }
public:
  void dispatch(Serializer& s) override {
    std::function<void(Serializer&)> cb;
    {
      std::unique_lock<std::mutex> lock(cbM);
      size_t id = read<size_t>(s);
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
  std::shared_ptr<Connection> conn;
private:
  size_t requestId = 0;
  std::map<size_t, std::function<void(Serializer&)>> callbacks;
  std::mutex cbM;
};

template <typename T>
std::shared_ptr<T> Connection::getProxyFor() 
{
  std::unique_lock<std::mutex> lock(m);
  std::shared_ptr<T> base = std::dynamic_pointer_cast<T>(proxies[interfaceNameOf<T>()]);
  if (base) 
    return base;

  std::shared_ptr<ProxyBase<T>> newProxy = ProxyBase<T>::Create(lock, shared_from_this());
  if (!newProxy)
    throw remote_exception("Connection does not implement proxy, or connection dropped early");

  proxies[interfaceNameOf<T>()] = newProxy;
  return newProxy;
}

#define PROXY_PROLOG(name, type)  \
  if (!conn) return async([]() -> type {throw remote_exception("Remote end disconnected prematurely"); }); \
  size_t reqId = getRequestId(); \
  future<type> f = getFutureFor<type>(reqId); \
  Serializer s; \
  write(s, "d" + getInterfaceName()); \
  write(s, std::string(#name)); \
  write(s, reqId);
#define PROXY_EPILOG(type) \
  conn->send(s); \
  return f;
#define PROXY_FUNC0(name, type) future<type> name() override { PROXY_PROLOG(name, type) PROXY_EPILOG(type) }
#define PROXY_FUNC1(name, type, a1) future<type> name(a1 A1) override { PROXY_PROLOG(name, type) write(s, A1); PROXY_EPILOG(type) }
#define PROXY_FUNC2(name, type, a1, a2) future<type> name(a1 A1, a2 A2) override { PROXY_PROLOG(name, type) write(s, A1); write(s, A2); PROXY_EPILOG(type) }
#define PROXY_FUNC3(name, type, a1, a2, a3) future<type> name(a1 A1, a2 A2, a3 A3) override { PROXY_PROLOG(name, type) write(s, A1); write(s, A2); write(s, A3); PROXY_EPILOG(type) }
#define PROXY_FUNC4(name, type, a1, a2, a3, a4) future<type> name(a1 A1, a2 A2, a3 A3, a4 A4) override { PROXY_PROLOG(name, type) write(s, A1); write(s, A2); write(s, A3); write(s, A4); PROXY_EPILOG(type) }
#define PROXY_FUNC5(name, type, a1, a2, a3, a4, a5) future<type> name(a1 A1, a2 A2, a3 A3, a4 A4, a5 A5) override { PROXY_PROLOG(name, type) write(s, A1); write(s, A2); write(s, A3); write(s, A4); write(s, A5); PROXY_EPILOG(type) }
#define PROXY_FUNC6(name, type, a1, a2, a3, a4, a5, a6) future<type> name(a1 A1, a2 A2, a3 A3, a4 A4, a5 A5, a6 A6) override { PROXY_PROLOG(name, type) write(s, A1); write(s, A2); write(s, A3); write(s, A4); write(s, A5); write(s, A6); PROXY_EPILOG(type) }
#define PROXY_FUNC7(name, type, a1, a2, a3, a4, a5, a6, a7) future<type> name(a1 A1, a2 A2, a3 A3, a4 A4, a5 A5, a6 A6, a7 A7) override { PROXY_PROLOG(name, type) write(s, A1); write(s, A2); write(s, A3); write(s, A4); write(s, A5); write(s, A6); write(s, A7); PROXY_EPILOG(type) }

class IHasDispatch {
public:
  virtual void dispatch(Serializer&s, std::shared_ptr<Connection> c) = 0;
};

template <typename T>
class DispatchBase : public IHasDispatch{
public:
  static void Provide(T *n);
protected:
  std::string interfaceName;
  std::string getInterfaceName() { 
    return interfaceName;
  }
  DispatchBase(T *cb)
  : interfaceName(interfaceNameOf<T>())
  , cb(cb)
  {
    Connection::dispatchers()["d" + getInterfaceName()] = this;
  }
  void dispatch(Serializer& s, std::shared_ptr<Connection> c) {
    std::string name = read<std::string>(s);
    auto it = funcs.find(name);
    if (it == funcs.end())
      printf("No function %s found on interface %s\n", name.c_str(), getInterfaceName().c_str());
    else
      it->second(s, c);
  }
  std::map<std::string, std::function<void(Serializer&, std::shared_ptr<Connection>)>> funcs;
  T *cb;
};

#define DISPATCH_PROLOG(name)  \
  funcs[#name] = [this](Serializer&s, std::shared_ptr<Connection> c){ \
  size_t reqId = read<size_t>(s); 
#define DISPATCH_EPILOG(rv)  \
  val.then([c, this, reqId](const future<rv> &v){ \
    Serializer s2; \
    write(s2, "p" + getInterfaceName()); \
    write(s2, reqId); \
    write(s2, v.get()); \
    c->send(s2); \
  }); \
  }
#define DISPATCH_EPILOGv(rv)  \
  val.then([c, this, reqId](const future<rv> &){ \
    Serializer s2; \
    write(s2, "p" + getInterfaceName()); \
    write(s2, reqId); \
    c->send(s2); \
  }); \
  }
#define DISPATCH_FUNC0(name, rv) DISPATCH_PROLOG(name) \
  future<rv> val = this->cb->name(); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC0v(name, rv) DISPATCH_PROLOG(name) \
  future<rv> val = this->cb->name(); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC1(name, rv, A1) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  future<rv> val = this->cb->name(a1); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC1v(name, rv, A1) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  future<rv> val = this->cb->name(a1); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC2(name, rv, A1, A2) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  future<rv> val = this->cb->name(a1, a2); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC2v(name, rv, A1, A2) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  future<rv> val = this->cb->name(a1, a2); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC3(name, rv, A1, A2, A3) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  future<rv> val = this->cb->name(a1, a2, a3); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC3v(name, rv, A1, A2, A3) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  future<rv> val = this->cb->name(a1, a2, a3); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC4(name, rv, A1, A2, A3, A4) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  A4 a4 = read<A4>(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC4v(name, rv, A1, A2, A3, A4) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  A4 a4 = read<A4>(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC5(name, rv, A1, A2, A3, A4, A5) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  A4 a4 = read<A4>(s); \
  A5 a5 = read<A5>(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC5v(name, rv, A1, A2, A3, A4, A5) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  A4 a4 = read<A4>(s); \
  A5 a5 = read<A5>(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC6(name, rv, A1, A2, A3, A4, A5, A6) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  A4 a4 = read<A4>(s); \
  A5 a5 = read<A5>(s); \
  A6 a6 = read<A6>(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5, a6); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC6v(name, rv, A1, A2, A3, A4, A5, A6) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  A4 a4 = read<A4>(s); \
  A5 a5 = read<A5>(s); \
  A6 a6 = read<A6>(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5, a6); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC7(name, rv, A1, A2, A3, A4, A5, A6, A7) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  A4 a4 = read<A4>(s); \
  A5 a5 = read<A5>(s); \
  A6 a6 = read<A6>(s); \
  A7 a7 = read<A7>(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5, a6, a7); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC7v(name, rv, A1, A2, A3, A4, A5, A6, A7) DISPATCH_PROLOG(name) \
  A1 a1 = read<A1>(s); \
  A2 a2 = read<A2>(s); \
  A3 a3 = read<A3>(s); \
  A4 a4 = read<A4>(s); \
  A5 a5 = read<A5>(s); \
  A6 a6 = read<A6>(s); \
  A7 a7 = read<A7>(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5, a6, a7); \
  DISPATCH_EPILOGv(rv)

#define REGISTER(Name) template <> std::shared_ptr<ProxyBase<Name>> ProxyBase<Name>::Create(std::unique_lock<std::mutex> &lock, std::shared_ptr<Connection> c) { \
                         c->cv.wait(lock, [c]{ return c->haveInterfaces; }); \
                         if (std::find(c->interfaces.begin(), c->interfaces.end(), interfaceNameOf<Name>()) != c->interfaces.end()) {\
                           return std::make_shared<Name##Proxy>(c); \
                         }\
                         return std::shared_ptr<Name##Proxy>(NULL); \
                       }\
                       template <> void DispatchBase<Name>::Provide(Name *n) { \
                         static Name##Dispatch __inst(n); \
                       }


#endif



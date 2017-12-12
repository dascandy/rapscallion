#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include "InterfaceProxy.h"
#include "Connection.h"

namespace Rapscallion {

class Connection;
class InterfaceProxy;

struct RpcClient {
  RpcClient(std::shared_ptr<Connection> connection) 
  : connection(std::move(connection))
  {}
  ~RpcClient() {
    for (auto& proxy : proxies) {
      proxy->signalDisconnect();
    }
  }
  template <typename T>
  T* Get() {
    T* proxy = new typename T::Proxy(connection);
    RegisterProxy(proxy);
    return proxy;
  }
  void Send(const Serializer& s) {
    connection->write(reinterpret_cast<const char*>(s.data()), s.size());
  }
  // Lots of TODO here
  std::vector<InterfaceProxy*> proxies;
  std::shared_ptr<Connection> connection;
};

}



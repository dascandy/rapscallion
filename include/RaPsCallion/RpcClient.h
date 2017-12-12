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
  RpcClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
  : connection_(std::make_shared<Connection>(socket, [this](const char* data, size_t size){
    des.AddBytes(reinterpret_cast<const uint8_t*>(data), size);
    while (des.HasFullPacket()) {
      Handle();
      des.RemovePacket();
    }
  }))
  {
    printf("%s\n", __PRETTY_FUNCTION__);
    connection_->start();
  }
  ~RpcClient() {
    printf("%s\n", __PRETTY_FUNCTION__);
    for (auto& proxy : proxies) {
      proxy->signalDisconnect();
    }
  }
  template <typename T>
  T* Get() {
    printf("%s\n", __PRETTY_FUNCTION__);
    std::lock_guard<std::mutex> l(m);
    typename T::Proxy* proxy = new typename T::Proxy(*this);
    proxies.push_back(proxy);
    return proxy;
  }
  void Send(const Serializer& s) {
    printf("%s\n", __PRETTY_FUNCTION__);
    connection_->write(reinterpret_cast<const char*>(s.data()), s.size());
  }
  void Handle() {
    printf("%s\n", __PRETTY_FUNCTION__);
    std::lock_guard<std::mutex> l(m);
    std::string ifId = serializer<std::string>::read(des);
    for (auto& iface : proxies) {
      if (iface->getInterfaceName() == ifId) {
        iface->Handle(des);
        return;
      }
    }
  }
  std::mutex m;
  std::vector<InterfaceProxy*> proxies;
  std::shared_ptr<Connection> connection_;
  Deserializer des;
};

}



#pragma once

#include <boost/asio.hpp>
#include <boost/make_unique.hpp>
#include <mutex>
#include <vector>
#include <memory>
#include "Server.h"
#include "InterfaceDispatcher.h"

namespace Rapscallion {

struct InterfaceDispatcher;
struct RpcHandle;

struct RpcHost {
  RpcHost(boost::asio::io_service &io_service, uint16_t port)
  : server(io_service, port, [this](std::shared_ptr<boost::asio::ip::tcp::socket> s){ addSocket(s); })
  {}
  void addSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    std::lock_guard<std::mutex> l(m);
    std::shared_ptr<RpcHandle> handle = std::make_shared<RpcHandle>(*this, std::move(socket));
    handles.push_back(handle);
    for (auto& interface : interfaces) {
      handle->SendInterface(interface);
    }
  }
  template <typename T>
  void Register(T* handler) {
    std::lock_guard<std::mutex> l(m);
    interfaces.emplace_back(boost::make_unique<typename T::Dispatcher>(handler));
    for (auto& handle : handles) {
      handle->SendInterface(interfaces.back());
    }
  }
  void Handle(Deserializer& deserializer, RpcHandle& handle) {
    std::lock_guard<std::mutex> l(m);
    std::string ifId = serializer<std::string>::read(deserializer);
    printf("%s\n", __PRETTY_FUNCTION__);
    for (auto& iface : interfaces) {
      if (iface->getInterfaceName() == ifId) {
        iface->Handle(deserializer, handle);
        return;
      }
    }
    // LOG error somehow. 
  }
  std::mutex m;
  std::vector<std::unique_ptr<InterfaceDispatcher>> interfaces;
  std::vector<std::shared_ptr<RpcHandle>> handles;
  Server server;
};

}



#pragma once

#include <boost/asio.hpp>
#include <mutex>
#include <vector>
#include <memory>

namespace Rapscallion {

struct InterfaceStub;
struct RpcHandle;

struct RpcHost {
  RpcHost(boost::asio::io_service &io_service, uint16_t port)
  : server(io_service, port, [](std::shared_ptr<tcp::socket> s){ addSocket(s); })
  {}
  void addSocket(std::shared_ptr<tcp::socket> socket) {
    std::lock_guard<std::mutex> l(m);
    std::shared_ptr<RpcHandle> handle = std::make_shared<RpcHandle>(*this, std::move(socket));
    handles.push_back(handle);
    for (auto& interface : interfaces) {
      handle->sendInterface(interface);
    }
  }
  template <typename T>
  void Register(T* handler) {
    std::lock_guard<std::mutex> l(m);
    std::unique_ptr<InterfaceStub> iface = std::make_unique<T::Stub>(handler);
    RegisterStub(new T::Stub(handler));
    for (auto& handle : handles) {
      handle->sendInterface(interface);
    }
  }
  void Handle(Deserializer& deserializer, RpcHandle& handle) {
    std::lock_guard<std::mutex> l(m);
    int ifId = serializer<int>::read(deserializer);
    for (auto& iface : interfaces) {
      if (iface->getId() == ifId) {
        iface->Dispatch(deserializer, handle);
        return;
      }
    }
    // LOG error somehow. 
  }
  std::mutex m;
  std::vector<std::unique_ptr<InterfaceStub>> interfaces;
  std::vector<std::shared_ptr<RpcHandle> handles;
};

}



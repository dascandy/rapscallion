#pragma once

#include <boost/asio.hpp>
#include "Serializer.h"
#include <memory>

namespace Rapscallion {

class RpcHost;
class Connection;
class InterfaceDispatcher;

struct RpcHandle {
  RpcHandle(RpcHost& host, boost::asio::ip::tcp::socket sock);
  void SendInterface(std::unique_ptr<InterfaceDispatcher>& iface);
  void Send(Serializer& s);
  std::shared_ptr<Connection> conn;
  Deserializer des;
};

}



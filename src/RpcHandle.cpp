#include "RpcHandle.h"
#include "RpcHost.h"
#include "Connection.h"

namespace Rapscallion {

RpcHandle::RpcHandle(RpcHost& host, std::shared_ptr<boost::asio::ip::tcp::socket> sock)
: conn(std::make_shared<Connection>(sock, [&host, this](const char* data, size_t size){
  des.AddBytes(reinterpret_cast<const uint8_t*>(data), size);
  while (des.HasFullPacket()) {
    host.Handle(des, *this);
    des.RemovePacket();
  }
}))
{
}

void RpcHandle::SendInterface(std::unique_ptr<InterfaceDispatcher>& iface) {
  Serializer s;
  serializer<std::string>::write(s, "");
  serializer<std::string>::write(s, iface->getInterfaceName());
  Send(s);
}

void RpcHandle::Send(Serializer& s) {
  conn->write(reinterpret_cast<const char*>(s.data()), s.size());
}

}



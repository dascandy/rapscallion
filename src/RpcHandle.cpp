#include "RpcHandle.h"
#include "RpcHost.h"

RpcHandle::RpcHandle(RpcHost& host, std::shared_ptr<boost::asio::ip::tcp::socket> sock)
: conn(sock, [&host, this](const char* data, size_t size){
  des.AddBytes(reinterpret_cast<const uint8_t*>(data), size);
  while (des.HasFullPacket()) {
    host.Handle(deserializer, this);
    des.RemovePacket();
  }
})
{}

void RpcHandle::SendInterface(std::unique_ptr<InterfaceDispatcher>& iface) {
  Serializer s;
  s.write(0);
  s.write(iface.identifier());
  conn.write(s);
}

void RpcHandle::Send(Serializer& s) {
  conn->write(s.data(), s.size());
}



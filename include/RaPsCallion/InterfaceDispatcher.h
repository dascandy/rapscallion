#pragma once
#include <boost/asio.hpp>

namespace Rapscallion {

class Deserializer;
class RpcHandle;

struct InterfaceDispatcher {
  virtual ~InterfaceDispatcher() = default;
  virtual const std::string& getInterfaceName () = 0;
  virtual void Handle(Deserializer& des, RpcHandle& handle) = 0;
};

}



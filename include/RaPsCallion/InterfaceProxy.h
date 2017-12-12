#pragma once
#include <boost/asio.hpp>

namespace Rapscallion {

struct InterfaceProxy {
  virtual ~InterfaceProxy() = default;
  virtual const std::string& getInterfaceName () = 0;
  virtual void signalDisconnect() = 0;
  virtual void Handle(Deserializer& s) = 0;

};

}



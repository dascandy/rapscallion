#pragma once
#include <boost/asio.hpp>

namespace Rapscallion {

struct InterfaceProxy {
  virtual ~InterfaceProxy() = default;
  virtual void signalDisconnect() = 0;
};

}



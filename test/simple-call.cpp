#include <awesome/assert.hpp>
#include <cmath>
#include <iomanip>
#include <RaPsCallion/future.h>
#include <sstream>
#include <string>

namespace rpc {
  rapscallion::future<double> a(rapscallion::connection& c, rapscallion::future<int> i);
  rapscallion::future<std::string> b(rapscallion::connection& c, rapscallion::future<double> i);

  namespace impl {
    constexpr double a(int i) {
      return i * M_PI;
    }
    std::string b(double i) {
      std::ostringstream s;
      s << std::setw(16) << std::hexfloat << i;
      return s.str();
    }
  }
}

int main()
{
  rapscallion::connection c;
  auto x = rpc::a(c, 42);
  AWESOME_ASSERT(c.tx_bytes() == 0);
  auto y = rpc::b(c, x);
  AWESOME_ASSERT(c.tx_bytes() == 0);
  auto z = y.get_local();
  AWESOME_ASSERT(c.tx_bytes() > 0);
  AWESOME_ASSERT(c.rx_bytes() == 0);
  auto w = z.get();
  AWESOME_ASSERT(c.rx_bytes() > 0);
}

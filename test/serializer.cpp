#include <cassert>
#include <cmath>
#include <limits>
#include <serializer.h>

void test(const double in) {
  Serializer s;
  writer<decltype(+in)>::write(s, in);
  const auto buf = s.data();
  Deserializer d(s.buffer, s.buffer.size() - buf.second);
  const auto out = reader<decltype(+in)>::read(d);
  assert(in == out || (std::isnan(in) && std::isnan(out)));
}

int main() {
  test(std::numeric_limits<double>::infinity());
  test(-std::numeric_limits<double>::infinity());
  test(std::numeric_limits<double>::quiet_NaN());
  test(1.0);
  test(-1.0);
  test(2.0);
  test(-2.0);
  test(M_PI);
  test(-M_PI);
  test(std::numeric_limits<double>::epsilon());
  test(std::numeric_limits<double>::min());
  test(std::numeric_limits<double>::max());
  test(std::numeric_limits<double>::denorm_min());
}

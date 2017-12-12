#pragma once
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <typeinfo>
#include "InterfaceDispatcher.h"
#include "Serializer.h"
#include "RpcHandle.h"

namespace Rapscallion {

class RpcHandle;

template <typename T>
class DispatcherBase : public InterfaceDispatcher {
protected:
  DispatcherBase(T *cb)
  : cb(cb)
  {
  }
  static constexpr const char* interfaceNameOf() {
    const char *str = typeid(T).name();
    while (*str && *str <= '9') ++str;
    return str;
  }
  std::string interfaceName = interfaceNameOf();
  std::string getInterfaceName() { 
    return interfaceName;
  }
  void Handle(Deserializer& des, RpcHandle& handle) override {
    std::string name = serializer<std::string>::read(des);
    auto it = funcs.find(name);
    if (it == funcs.end())
      printf("No function %s found on interface %s\n", name.c_str(), getInterfaceName().c_str());
    else
      it->second(des, handle);
  }
  std::map<std::string, std::function<void(Deserializer&, RpcHandle&)>> funcs;
  T *cb;
};

#define DISPATCH_PROLOG(name)  \
  funcs[#name] = [this](Rapscallion::Deserializer&s, Rapscallion::RpcHandle& c){ \
  size_t reqId = Rapscallion::serializer<size_t>::read(s); 
#define DISPATCH_EPILOG(rv)  \
  val.then([handle = &c, this, reqId](future<rv> v){ \
    Rapscallion::Serializer s2; \
    Rapscallion::serializer<std::string>::write(s2, "p" + getInterfaceName()); \
    Rapscallion::serializer<size_t>::write(s2, reqId); \
    Rapscallion::serializer<rv>::write(s2, v.get()); \
    handle->Send(s2); \
  }); \
  }
#define DISPATCH_EPILOGv(rv)  \
  val.then([handle = &c, this, reqId](future<rv> ){ \
    Rapscallion::Serializer s2; \
    Rapscallion::serializer<std::string>::write(s2, "p" + getInterfaceName()); \
    Rapscallion::serializer<size_t>::write(s2, reqId); \
    handle->Send(s2); \
  }); \
  }
#define DISPATCH_FUNC0(name, rv) DISPATCH_PROLOG(name) \
  future<rv> val = this->cb->name(); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC0v(name, rv) DISPATCH_PROLOG(name) \
  future<rv> val = this->cb->name(); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC1(name, rv, A1) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  future<rv> val = this->cb->name(a1); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC1v(name, rv, A1) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  future<rv> val = this->cb->name(a1); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC2(name, rv, A1, A2) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  future<rv> val = this->cb->name(a1, a2); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC2v(name, rv, A1, A2) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  future<rv> val = this->cb->name(a1, a2); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC3(name, rv, A1, A2, A3) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC3v(name, rv, A1, A2, A3) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC4(name, rv, A1, A2, A3, A4) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  A4 a4 = Rapscallion::serializer<A4>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC4v(name, rv, A1, A2, A3, A4) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  A4 a4 = Rapscallion::serializer<A4>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC5(name, rv, A1, A2, A3, A4, A5) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  A4 a4 = Rapscallion::serializer<A4>::read(s); \
  A5 a5 = Rapscallion::serializer<A5>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC5v(name, rv, A1, A2, A3, A4, A5) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  A4 a4 = Rapscallion::serializer<A4>::read(s); \
  A5 a5 = Rapscallion::serializer<A5>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC6(name, rv, A1, A2, A3, A4, A5, A6) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  A4 a4 = Rapscallion::serializer<A4>::read(s); \
  A5 a5 = Rapscallion::serializer<A5>::read(s); \
  A6 a6 = Rapscallion::serializer<A6>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5, a6); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC6v(name, rv, A1, A2, A3, A4, A5, A6) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  A4 a4 = Rapscallion::serializer<A4>::read(s); \
  A5 a5 = Rapscallion::serializer<A5>::read(s); \
  A6 a6 = Rapscallion::serializer<A6>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5, a6); \
  DISPATCH_EPILOGv(rv)
#define DISPATCH_FUNC7(name, rv, A1, A2, A3, A4, A5, A6, A7) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  A4 a4 = Rapscallion::serializer<A4>::read(s); \
  A5 a5 = Rapscallion::serializer<A5>::read(s); \
  A6 a6 = Rapscallion::serializer<A6>::read(s); \
  A7 a7 = Rapscallion::serializer<A7>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5, a6, a7); \
  DISPATCH_EPILOG(rv)
#define DISPATCH_FUNC7v(name, rv, A1, A2, A3, A4, A5, A6, A7) DISPATCH_PROLOG(name) \
  A1 a1 = Rapscallion::serializer<A1>::read(s); \
  A2 a2 = Rapscallion::serializer<A2>::read(s); \
  A3 a3 = Rapscallion::serializer<A3>::read(s); \
  A4 a4 = Rapscallion::serializer<A4>::read(s); \
  A5 a5 = Rapscallion::serializer<A5>::read(s); \
  A6 a6 = Rapscallion::serializer<A6>::read(s); \
  A7 a7 = Rapscallion::serializer<A7>::read(s); \
  future<rv> val = this->cb->name(a1, a2, a3, a4, a5, a6, a7); \
  DISPATCH_EPILOGv(rv)

#define REGISTERDISPATCHER(Name) template <> void DispatchBase<Name>::Provide(Name *n) { \
                         static Name##Dispatch __inst(n); \
                       }

}


#include <RaPsCallion/future.h>
#include <string>
#include <tuple>
#include <vector>

namespace rpc {
  template <typename K, typename V>
  class map_view
  {
  public:
    const V& at(const K&) const;
  };

  rapscallion::future<map_view<std::string, std::vector<unsigned>>> enumerate_interfaces();
  rapscallion::future<bool> register_interface_at(
      rapscallion::future<std::string>                    interface
    , rapscallion::future<int>                            first_id
    , rapscallion::future<int>                            last_id
    );

  template <typename K, typename V>
  rapscallion::future<V> at(rapscallion::future<map_view<K, V>> map, rapscallion::future<K> key);

  template <typename K, typename V>
  rapscallion::future<V> at(rapscallion::future<map_view<K, V>> map, const K& key)
  {
    return at(map, rapscallion::future<K>(key));
  }

  template <typename K, typename V>
  rapscallion::future<V> at(const map_view<K, V>& map, const K& key)
  {
    return map.at(key);
  }

  template <typename K, typename V>
  std::future<V> at(const map_view<K, V>& map, rapscallion::future<K> key)
  {
    return key.get_local().then([&map](std::future<K> key) {
        return map.at(key.get());
      });
  }
}

int main()
{
  const std::string iface_name { "some.interface" };
  auto available_version = at(rpc::enumerate_interfaces(), iface_name);
  auto success = rpc::register_interface_at(iface_name, 3, 10);
  return success.get_local().get() ? 0 : 1;
}

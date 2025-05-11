#ifndef LIBTORRENT_HELPERS_MOCK_COMPARE_H
#define LIBTORRENT_HELPERS_MOCK_COMPARE_H

#include <algorithm>
#include <type_traits>
#include <torrent/event.h>
#include <torrent/net/socket_address.h>

// Compare arguments to mock functions with what is expected. The lhs
// are the expected arguments, rhs are the ones called with.

template <typename Arg>
inline bool mock_compare_arg(Arg lhs, Arg rhs) { return lhs == rhs; }

template <int I, typename A, typename... Args>
typename std::enable_if<I == 1, int>::type
mock_compare_tuple(const std::tuple<A, Args...>& lhs, const std::tuple<Args...>& rhs) {
  return mock_compare_arg(std::get<I>(lhs), std::get<I - 1>(rhs)) ? 0 : 1;
}

template <int I, typename A, typename... Args>
typename std::enable_if<1 < I, int>::type
mock_compare_tuple(const std::tuple<A, Args...>& lhs, const std::tuple<Args...>& rhs) {
  auto res = mock_compare_tuple<I - 1>(lhs, rhs);

  if (res != 0)
    return res;

  return mock_compare_arg(std::get<I>(lhs), std::get<I - 1>(rhs)) ? 0 : I;
}

//template <typename T, typename std::enable_if<!std::is_const<T>::value, int>::type = 0>
template <typename T>
struct mock_compare_map {
  typedef std::map<const T*, const T*> values_type;

  static T* begin_pointer() { return reinterpret_cast<T*>(0x1000); }
  static T* end_pointer() { return reinterpret_cast<T*>(0x2000); }

  static bool is_key(const T* k) {
    return k >= begin_pointer() && k < end_pointer();
  }

  static bool has_key(const T* k) {
    return values.find(k) != values.end();
  }

  static bool has_value(const T* v) {
    return std::find_if(values.begin(), values.end(), [v](typename values_type::value_type& kv) { return v == kv.second; }) != values.end();
  }

  static const T* get(const T* k) {
    auto itr = values.find(k);
    CPPUNIT_ASSERT_MESSAGE("mock_compare_map get failed, not inserted", itr != values.end());
    return itr->second;
  }

  static values_type values;
};

template<typename T>
typename mock_compare_map<T>::values_type mock_compare_map<T>::values;

template<typename T>
void mock_compare_add(T* v) {
  mock_compare_map<T>::add_value(v);
}

//
// Specialize:
//

template <>
inline bool mock_compare_arg<sockaddr*>(sockaddr* lhs, sockaddr* rhs) {
  return lhs != nullptr && rhs != nullptr && torrent::sa_equal(lhs, rhs);
}
template <>
inline bool mock_compare_arg<const sockaddr*>(const sockaddr* lhs, const sockaddr* rhs) {
  return lhs != nullptr && rhs != nullptr && torrent::sa_equal(lhs, rhs);
}

template <>
inline bool mock_compare_arg<torrent::Event*>(torrent::Event* lhs, torrent::Event* rhs) {
  if (mock_compare_map<torrent::Event>::is_key(lhs)) {
    if (!mock_compare_map<torrent::Event>::has_value(rhs)) {
      mock_compare_map<torrent::Event>::values[lhs] = rhs;
      return true;
    }

    return mock_compare_map<torrent::Event>::has_key(lhs) && mock_compare_map<torrent::Event>::get(lhs) == rhs;
  }

  return lhs == rhs;
}

#endif

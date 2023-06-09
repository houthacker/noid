/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_CONCEPTS_H_
#define NOID_SRC_BACKEND_CONCEPTS_H_

#include <concepts>
#include <type_traits>

template <typename T, typename V>
concept Container = requires(T obj, std::size_t i) {
  { obj.size() } -> std::convertible_to<std::size_t>;
  { obj.operator[](i) } -> std::convertible_to<V&>;
  { obj.data() } -> std::convertible_to<V*>;
};

template <typename T, typename V>
concept DynamicallySizedContainer = Container<T, V> && requires(T obj, std::size_t i) {
  { obj.resize(i) } -> std::same_as<std::void_t<V>>;
};

#endif //NOID_SRC_BACKEND_CONCEPTS_H_

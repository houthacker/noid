/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_CONCEPTS_H_
#define NOID_SRC_BACKEND_CONCEPTS_H_

#include <concepts>
#include <type_traits>

template<typename It>
concept LegacyIterator = std::is_copy_constructible_v<It>
    and std::is_copy_assignable_v<It>
    and std::is_destructible_v<It>
    and not std::is_same_v<It, std::void_t<It>>
    and requires(It iterator, It other) {
      { std::swap(iterator, other) } -> std::same_as<std::void_t<It>>;
      { ++iterator } -> std::same_as<It&>;
    };

template<typename It>
concept LegacyInputIterator = LegacyIterator<It>
    and std::equality_comparable<It>
    and requires(const It i, const It j, It r) {
      { i != j } -> std::convertible_to<bool>;
      { *i } -> std::same_as<typename std::iterator_traits<It>::reference>;
      { *i } -> std::convertible_to<typename std::iterator_traits<It>::value_type>;
      { *r++ } -> std::convertible_to<typename std::iterator_traits<It>::value_type>;
    };

template<typename It>
concept LegacyForwardIterator = LegacyInputIterator<It>
    and std::is_default_constructible_v<It>
    and (
        (std::is_same_v<typename std::iterator_traits<It>::reference, typename std::iterator_traits<It>::value_type&>
          or std::is_same_v<typename std::iterator_traits<It>::reference, typename std::iterator_traits<It>::value_type&&>)

        or
        (std::is_same_v<typename std::iterator_traits<It>::reference, const typename std::iterator_traits<It>::value_type&>
            or std::is_same_v<typename std::iterator_traits<It>::reference, const typename std::iterator_traits<It>::value_type&&>)
    )
    and requires(It iterator) {
      { iterator++ } -> std::same_as<It>;
      { *iterator++ } -> std::same_as<typename std::iterator_traits<It>::reference>;
    };

template<typename T, typename V>
concept Container = std::same_as<typename T::value_type, V>
    and std::same_as<typename T::reference, V&>
    and std::same_as<typename T::const_reference, const V&>
    and LegacyForwardIterator<typename T::iterator>
    and LegacyForwardIterator<typename T::const_iterator>
    and std::same_as<typename T::difference_type, typename std::iterator_traits<typename T::iterator>::difference_type>
    and std::convertible_to<typename T::size_type, typename T::difference_type>
    and requires(T obj, std::size_t i) {
      { obj.size() } -> std::same_as<typename T::size_type>;
      { obj.max_size() } -> std::same_as<typename T::size_type>;
      { obj.empty() } -> std::convertible_to<bool>;
      { obj.operator[](i) } -> std::convertible_to<V&>;
      { obj.data() } -> std::convertible_to<V*>;
    };

template<typename T, typename V>
concept DynamicallySizedContainer = Container<T, V> && requires(T obj, std::size_t i) {
  { obj.resize(i) } -> std::same_as<std::void_t<V>>;
};

#endif //NOID_SRC_BACKEND_CONCEPTS_H_

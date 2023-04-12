#ifndef NOID_SRC_STORAGE_ALGORITHM_H_
#define NOID_SRC_STORAGE_ALGORITHM_H_

#include <array>
#include <cstdint>
#include <functional>
#include <vector>
#include <memory>

#include "Shared.h"
#include "KeyBearer.h"

namespace noid::storage {

/**
 * @brief Recursively searches for the given needle and returns its index within the haystack.
 *
 * @tparam T The haystack element type.
 * @tparam Func The type of a function that, given a @c &T, returns the key @c &K it contains.
 * @param haystack The vector of @c std::unique_ptr containing the elements that must be searched.
 * @param low The lowest index to search.
 * @param high The highest index to search.
 * @param needle The needle that must be found.
 * @param get_key_reference The function which returns a @c &K,
 * @return The index at which the given needle resides, or -1 if no such element exists.
 */
template<typename T, typename Func>
int64_t BinarySearch(const std::vector<std::unique_ptr<T>>& haystack, int64_t low,
                     int64_t high, const K& needle, Func get_key_reference) {
  if (high >= low) {
    auto middle_index = low + (high - low) / 2;
    auto& middle = get_key_reference(*haystack[middle_index]);

    if (middle == needle) {
      return middle_index;
    } else if (middle < needle) {
      // search right
      return BinarySearch(haystack, middle_index + 1, high, needle, get_key_reference);
    }

    // search left
    return BinarySearch(haystack, low, middle_index - 1, needle, get_key_reference);
  }

  return -1;
}

/**
 * @brief Recursively searches the greatest element that might equal but not exceed the given needle.
 * @note The needle itself does not need to reside in the haystack. This allows for searching sparse haystacks.
 *
 * @tparam T The haystack element type.
 * @tparam Func The type of function that, given a @c &T, returns the key @c &K it contains.
 * @param haystack The vector of @c std::unique_ptr containing the elements that must be searched.
 * @param low The lowest index to search.
 * @param high The highest index to search.
 * @param needle The needle that must be found.
 * @param get_key_reference The function which returns a @c &K,
 * @return The index at which the searched element resides, or -1 if no such element exists.
 */
template<typename T, typename Func>
int64_t GreatestNotExceeding(const std::vector<std::unique_ptr<T>>& haystack, int64_t low,
                             int64_t high, const K& needle, Func get_key_reference) {
  auto middle_index = low + (high - low) / 2;
  auto& middle = get_key_reference(*haystack[middle_index]);

  if (middle_index == low && needle < middle /* use instead of middle > needle to prevent implementation of an extra operator */ ) {
    return -1;
  }

  auto is_candidate = middle < needle || middle == needle;
  if (is_candidate && (middle_index == high || needle < get_key_reference(*haystack[middle_index + 1]) /* invert operator to prevent implementation of an extra operator */ )) {
    return middle_index;
  }

  if (is_candidate) {
    return GreatestNotExceeding(haystack, middle_index + 1, high, needle, get_key_reference);
  }

  return GreatestNotExceeding(haystack, low, middle_index, needle, get_key_reference);
}

}

#endif //NOID_SRC_STORAGE_ALGORITHM_H_

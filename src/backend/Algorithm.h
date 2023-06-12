/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_ALGORITHM_H
#define NOID_ALGORITHM_H

#include <limits>

#include "Concepts.h"
#include "Types.h"

namespace noid::backend {


/**
 * @brief Recursively searches for the given needle and returns its index within the haystack.
 *
 * @tparam K The container element type.
 * @tparam C The container type.
 * @param haystack The container with the @c KeyBearers to be searched.
 * @param low The lowest index to search.
 * @param high The highest index to search.
 * @param needle The needle that must be found.
 * @return The index at which the given needle resides, or @c haystack.size() if no such element exists.
 */
template<KeyBearer<SearchKey> K, Container<K> C>
C::size_type BinarySearch(const C& haystack, typename C::size_type low,
    typename C::size_type high, const SearchKey& needle)
{
  if (high >= low) {
    typename C::size_type middle_index = low + (high - low) / 2;
    const auto& middle_key = haystack[middle_index].GetKey();

    if (middle_key == needle) {
      return middle_index;
    } else if (middle_key < needle) {

      // Adding 1 to the max value of C::size_type does not work for us so in that case we return 'not found'
      if (middle_index == std::numeric_limits<typename C::size_type>::max()) {
        return haystack.size();
      }

      // Search right
      return BinarySearch<K, C>(haystack, middle_index + 1, high, needle);
    }

    // Subtracting 1 from the min value of C::size_type does not work for us so in that case we return 'not found'
    if (middle_index == std::numeric_limits<typename C::size_type>::min()) {
      return haystack.size();
    }

    // Search left
    return BinarySearch<K, C>(haystack, low, middle_index - 1, needle);
  }

  return haystack.size();
}

}

#endif //NOID_ALGORITHM_H

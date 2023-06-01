/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_BITS_H_
#define NOID_SRC_BACKEND_BITS_H_

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "Types.h"
#include "Concepts.h"

namespace noid::backend {

static constexpr uint8_t uint8_max = std::numeric_limits<uint8_t>::max();

static constexpr uint16_t uint16_max_p2 = 32768; // 2^15

/**
 * Rounds @p value to the next power of two with an upper bound of 2^15.
 *
 * @param value The value to round up.
 * @return The next power of two, or 2^15 if @p value exceeds 2^15.
 */
inline uint16_t safe_round_to_next_power_of_2(uint16_t value) {
  if (value <= 2) {
    return 2;
  } else if (value > uint16_max_p2) {
    return uint16_max_p2;
  } else if (std::has_single_bit(value)) {
    return value;
  }

  auto log = static_cast<uint16_t>(log2(value));
  return static_cast<uint16_t>(pow(2, log)) << 1;
}

/**
 * Rounds the value up to the next multiple of 8 with an upper bound of @c std::numeric_limits<uint8_t>::max().
 *
 * @param value The value to round up.
 * @return The next multiple of 8, or @p value if it itself is a multiple of 8.
 */
inline uint8_t safe_next_multiple_of_8(uint8_t value) {
  if (value < 8) {
    return 8;
  }

  uint8_t remainder = value % 8;
  if (remainder > 0) {
    uint8_t to_add = 8 - remainder;

    uint16_t result = value + to_add;
    return result > uint8_max ? uint8_max : static_cast<uint8_t>(result);
  }

  return value;
}

/**
 * @brief Reads a @c uint8_t from @p haystack.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The container to read from.
 * @param read_idx The index to read the value from.
 * @return The @c uint8_t.
 * @throws std::out_of_range If reading from the given index causes a read outside of the array bounds.
 */
template<typename T, Container<T> Container>
inline uint8_t read_uint8(Container const &haystack, typename Container::size_type read_idx) {
  if (read_idx >= haystack.size()) {
    throw std::out_of_range("read_idx too large");
  }

  return (uint8_t)haystack[read_idx];
}

/**
 * @brief Writes a @c uint8_t to @p haystack.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The container to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 * @throws std::out_of_range If writing to the given index causes a write outside of the array bounds.
 */
template<typename T, Container<T> Container>
inline Container& write_uint8(Container &haystack, typename Container::size_type write_idx, uint8_t value) {
  if (write_idx >= haystack.size()) {
    throw std::out_of_range("write_idx too large");
  }

  haystack[write_idx] = (byte)value;

  return haystack;
}

/**
 * @brief Writes a @c uint8_t to @p haystack.
 * @note The container is asked to allocate more space if the given value cannot be written otherwise.
 *
 * @tparam T The container element type.
 * @tparam DynamicallySizedContainer The container type.
 * @param haystack The container to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 */
template<typename T, DynamicallySizedContainer<T> Container>
inline Container& write_uint8(Container &haystack, typename Container::size_type write_idx, uint8_t value) {
  if (write_idx + sizeof(uint8_t) > haystack.size()) {
    haystack.resize(write_idx = sizeof(uint8_t));
  }

  haystack[write_idx] = (byte)value;

  return haystack;
}

/**
 * @brief Reads a @c uint16_t from @p haystack.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The array to read from.
 * @param read_idx The index to start reading at.
 * @return The @c uint16_t
 * @throws std::out_of_range If reading the @c uint16_t would cause a read outside of the array bounds.
 */
template<typename T, Container<T> Container>
uint16_t read_le_uint16(Container const &haystack, typename Container::size_type read_idx) {
  if (read_idx > haystack.size() - sizeof(uint16_t)) {
    throw std::out_of_range("read_idx too large");
  }

  union {
      byte b[sizeof(uint16_t)];
      uint16_t i;
  } u = {{haystack[read_idx], haystack[read_idx + 1]}};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  return u.i;
}

/**
 * @brief Writes a @c uint16_t to @p haystack.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 * @throws std::out_of_range If writing @p value to @p haystack would cause a write outside of the container bounds.
 */
template<typename T, Container<T> Container>
Container& write_le_uint16(Container &haystack, typename Container::size_type write_idx, uint16_t value) {
  if (write_idx > haystack.size() - sizeof(uint16_t)) {
    throw std::out_of_range("write_idx too large");
  }

  union {
      uint16_t i;
      byte b[sizeof(uint16_t)];
  } u = {value};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  haystack[write_idx] = u.b[0];
  haystack[write_idx + 1] = u.b[1];

  return haystack;
}

/**
 * @brief Writes a @c uint16_t to @p haystack.
 * @note The container is asked to allocate more space if the given value cannot be written otherwise.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 */
template<typename T, DynamicallySizedContainer<T> Container>
Container& write_le_uint16(Container &haystack, typename Container::size_type write_idx, uint16_t value) {
  if (write_idx + sizeof(uint16_t) > haystack.size()) {
    haystack.resize(write_idx + sizeof(uint16_t));
  }

  union {
      uint16_t i;
      byte b[sizeof(uint16_t)];
  } u = {value};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  haystack[write_idx] = u.b[0];
  haystack[write_idx + 1] = u.b[1];

  return haystack;
}

/**
 * @brief Reads a @c uint32_t from @p haystack.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The container to read from.
 * @param read_idx The index to start reading at.
 * @return The @c uint32_t
 * @throws std::out_of_range If reading the @c uint32_t would cause a read outside of the container bounds.
 */
template<typename T, Container<T> Container>
inline uint32_t read_le_uint32(Container const &haystack, typename Container::size_type read_idx) {
  if (read_idx > haystack.size() - sizeof(uint32_t)) {
    throw std::out_of_range("read_idx too large");
  }

  union {
      byte b[sizeof(uint32_t)];
      uint32_t i;
  } u = {{haystack[read_idx], haystack[read_idx + 1], haystack[read_idx + 2], haystack[read_idx + 3]}};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  return u.i;
}

/**
 * @brief Writes a @c uint32_t to @p haystack.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 * @throws std::out_of_range If writing @p value to @p haystack would cause a write outside of the array bounds.
 */
template<typename T, Container<T> Container>
Container& write_le_uint32(Container &haystack, typename Container::size_type write_idx, uint32_t value) {
  if (write_idx > haystack.size() - sizeof(uint32_t)) {
    throw std::out_of_range("write_idx too large");
  }

  union {
      uint32_t i;
      byte b[sizeof(uint32_t)];
  } u = {value};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  haystack[write_idx] = u.b[0];
  haystack[write_idx + 1] = u.b[1];
  haystack[write_idx + 2] = u.b[2];
  haystack[write_idx + 3] = u.b[3];

  return haystack;
}

/**
 * @brief Writes a @c uint32_t to @p haystack.
 * @note The container is asked to allocate more space if the given value cannot be written otherwise.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 */
template<typename T, DynamicallySizedContainer<T> Container>
Container& write_le_uint32(Container &haystack, typename Container::size_type write_idx, uint32_t value) {
  if (write_idx + sizeof(uint32_t) > haystack.size()) {
    haystack.resize(write_idx + sizeof(uint32_t));
  }

  union {
      uint32_t i;
      byte b[sizeof(uint32_t)];
  } u = {value};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  haystack[write_idx] = u.b[0];
  haystack[write_idx + 1] = u.b[1];
  haystack[write_idx + 2] = u.b[2];
  haystack[write_idx + 3] = u.b[3];

  return haystack;
}

/**
 * @brief Reads a @c uint64_t from @p haystack.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The array to read from.
 * @param read_idx The index to start reading at.
 * @return The @c uint64_t
 * @throws std::out_of_range If reading the @c uint64_t would cause a read outside of the array bounds.
 */
template<typename T, Container<T> Container>
uint64_t read_le_uint64(Container const &haystack, typename Container::size_type read_idx) {
  if (read_idx > haystack.size() - sizeof(uint64_t)) {
    throw std::out_of_range("read_idx too large");
  }

  union {
      byte b[sizeof(uint64_t)];
      uint64_t i;
  } u = {{haystack[read_idx], haystack[read_idx + 1], haystack[read_idx + 2], haystack[read_idx + 3],
          haystack[read_idx + 4], haystack[read_idx + 5], haystack[read_idx + 6], haystack[read_idx + 7]}};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  return u.i;
}

/**
 * @brief Writes a @c uint64_t @p haystack.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 * @throws std::out_of_range If writing the value to @p haystack would cause a write outside of the array bounds.
 */
template<typename T, Container<T> Container>
Container& write_le_uint64(Container &haystack, typename Container::size_type write_idx, uint64_t value) {
  if (write_idx > haystack.size() - sizeof(uint64_t)) {
    throw std::out_of_range("write_idx too large");
  }

  union {
      uint64_t i;
      byte b[sizeof(uint64_t)];
  } u = {value};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  haystack[write_idx] = u.b[0];
  haystack[write_idx + 1] = u.b[1];
  haystack[write_idx + 2] = u.b[2];
  haystack[write_idx + 3] = u.b[3];
  haystack[write_idx + 4] = u.b[4];
  haystack[write_idx + 5] = u.b[5];
  haystack[write_idx + 6] = u.b[6];
  haystack[write_idx + 7] = u.b[7];

  return haystack;
}

/**
 * @brief Writes a @c uint64_t @p haystack.
 * @note The container is asked to allocate more space if the given value cannot be written otherwise.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 */
template<typename T, DynamicallySizedContainer<T> Container>
Container& write_le_uint64(Container &haystack, typename Container::size_type write_idx, uint64_t value) {
  if (write_idx + sizeof(uint64_t) > haystack.size()) {
    haystack.resize(write_idx + sizeof(uint64_t));
  }

  union {
      uint64_t i;
      byte b[sizeof(uint64_t)];
  } u = {value};

  if constexpr (std::endian::native == std::endian::big) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  haystack[write_idx] = u.b[0];
  haystack[write_idx + 1] = u.b[1];
  haystack[write_idx + 2] = u.b[2];
  haystack[write_idx + 3] = u.b[3];
  haystack[write_idx + 4] = u.b[4];
  haystack[write_idx + 5] = u.b[5];
  haystack[write_idx + 6] = u.b[6];
  haystack[write_idx + 7] = u.b[7];

  return haystack;
}

/**
 * @brief Creates a new container and copies data from @p source into it.
 *
 * @tparam T The container element type.
 * @tparam Destination The destination container type.
 * @tparam Source The source container type.
 * @param source The source container.
 * @param read_idx The index at which to start reading.
 * @param amount The amount of element to read.
 * @return The container instance (which resides on that stack).
 * @throws std::out_of_range if the given parameters would cause a read outside of the source bounds.
 */
template<typename T, Container<T> Destination, Container<T> Source>
Destination read_container(const Source &source, typename Source::size_type read_idx, typename Source::size_type amount) {
  Destination destination;
  if (read_idx + amount >= source.size()) {
    throw std::out_of_range("reading amount elements starting at read_idx causes a read outside of the source bounds.");
  }

  std::copy(std::begin(source) + read_idx, std::begin(source) + read_idx + amount, std::begin(destination));
  return destination;
}

/**
 * @brief Writes all data from @p source into @p destination, starting at @p write_idx.
 *
 * @tparam T The container element type.
 * @tparam Destination The destination container type.
 * @tparam Source The source container type.
 * @param destination The destination container.
 * @param write_idx The index to start writing at.
 * @param source The source container.
 * @return A reference to the destination container.
 * @throws std::out_of_range if source doesn't fit into destination using the given @p write_idx.
 */
template<typename T, Container<T> Destination, Container<T> Source>
Destination& write_container(Destination &destination, typename Destination::size_type write_idx, const Source &source) {
  if (source.size() > write_idx + destination.size()) {
    throw std::out_of_range("Cannot fit source into destination");
  }

  std::copy(std::begin(source), std::end(source), std::begin(destination) + write_idx);

  return destination;
}

/**
 * @brief Calculates the FNV-1a hash of @p length message bytes starting at @p start_idx.
 *
 * @tparam T The container element type.
 * @tparam Container The container type.
 * @param message The message.
 * @param start_idx The index to start hashing from.
 * @param length The amount of bytes to be hashed.
 * @return The FNV-1a hash.
 */
template<typename T, Container<T> Container>
uint32_t fnv1a(Container const &message, typename Container::size_type start_idx, typename Container::size_type length) {
  if (length > message.size() - start_idx) {
    throw std::out_of_range("length too large");
  }

  uint32_t hash = 2166136261 /* fnv offset basis */;

  for (auto i = start_idx; i < start_idx + length; i++) {
    hash ^= message[i];
    hash *= 16777619 /* fnv prime */;
  }

  return hash;
}

}
#endif //NOID_SRC_BACKEND_BITS_H_

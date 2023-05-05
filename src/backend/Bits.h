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

#include "Types.h"

namespace noid::backend {

enum class Endianness {
    BigEndian,
    LittleEndian,
};

static constexpr uint8_t uint8_max = std::numeric_limits<uint8_t>::max();

static constexpr uint16_t uint16_max_p2 = 32768; // 2^15

/**
 * @return The endianness of the current system.
 */
inline Endianness native_endianness() {
  union {
    uint32_t i;
    char c [4];
  } u = {0x01020304};

  return u.c[0] == 1 ? Endianness::BigEndian : Endianness::LittleEndian;
}

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
 * @tparam N The array size in bytes.
 * @param haystack The array to read from.
 * @param read_idx The index to read the value from.
 * @return The @c uint8_t.
 * @throws std::out_of_range If reading from the given index causes a read outside of the array bounds.
 */
template<std::size_t N>
inline uint8_t read_uint8(std::array<byte, N> const &haystack, typename std::array<byte, N>::size_type read_idx) {
  if (read_idx >= haystack.size()) {
    throw std::out_of_range("read_idx too large");
  }

  return (uint8_t)haystack[read_idx];
}

/**
 * @brief Writes a @c uint8_t to @p haystack.
 *
 * @tparam N The array size in bytes.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference @p haystack.
 */
template<std::size_t N>
inline std::array<byte, N>& write_uint8(std::array<byte, N> &haystack, typename std::array<byte, N>::size_type write_idx, uint8_t value) {
  if (write_idx >= haystack.size()) {
    throw std::out_of_range("write_idx too large");
  }

  haystack[write_idx] = (byte)value;

  return haystack;
}

/**
 * @brief Reads a @c uint16_t from @p haystack.
 *
 * @tparam N The array size in bytes.
 * @param haystack The array to read from.
 * @param read_idx The index to start reading at.
 * @return The @c uint16_t
 * @throws std::out_of_range If reading the @c uint16_t would cause a read outside of the array bounds.
 */
template<std::size_t N>
uint16_t read_le_uint16(std::array<byte, N> const &haystack, typename std::array<byte, N>::size_type read_idx) {
  if (read_idx > haystack.size() - sizeof(uint16_t)) {
    throw std::out_of_range("read_idx too large");
  }

  union {
      byte b[sizeof(uint16_t)];
      uint16_t i;
  } u = {{haystack[read_idx], haystack[read_idx + 1]}};

  if (native_endianness() == Endianness::BigEndian) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  return u.i;
}

/**
 * @brief Writes a @c uint16_t to @p haystack.
 *
 * @tparam N The array size in bytes.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 * @throws std::out_of_range If writing @p value to @p haystack would cause a write outside of the array bounds.
 */
template<std::size_t N>
std::array<byte, N>& write_le_uint16(std::array<byte, N> &haystack, typename std::array<byte, N>::size_type write_idx, uint16_t value) {
  if (write_idx > haystack.size() - sizeof(uint16_t)) {
    throw std::out_of_range("write_idx too large");
  }

  union {
      uint16_t i;
      byte b[sizeof(uint16_t)];
  } u = {value};

  if (native_endianness() == Endianness::BigEndian) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  haystack[write_idx] = u.b[0];
  haystack[write_idx + 1] = u.b[1];

  return haystack;
}

/**
 * @brief Reads a @c uint32_t from @p haystack.
 *
 * @tparam N The array size in bytes.
 * @param haystack The array to read from.
 * @param read_idx The index to start reading at.
 * @return The @c uint32_t
 * @throws std::out_of_range If reading the @c uint32_t would cause a read outside of the array bounds.
 */
template<std::size_t N>
uint32_t read_le_uint32(std::array<byte, N> const &haystack, typename std::array<byte, N>::size_type read_idx) {
  if (read_idx > haystack.size() - sizeof(uint32_t)) {
    throw std::out_of_range("read_idx too large");
  }

  union {
      byte b[sizeof(uint32_t)];
      uint32_t i;
  } u = {{haystack[read_idx], haystack[read_idx + 1], haystack[read_idx + 2], haystack[read_idx + 3]}};

  if (native_endianness() == Endianness::BigEndian) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  return u.i;
}

/**
 * @brief Writes a @c uint32_t to @p haystack.
 *
 * @tparam N The array size in bytes.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 * @throws std::out_of_range If writing @p value to @p haystack would cause a write outside of the array bounds.
 */
template<std::size_t N>
std::array<byte, N>& write_le_uint32(std::array<byte, N> &haystack, typename std::array<byte, N>::size_type write_idx, uint32_t value) {
  if (write_idx > haystack.size() - sizeof(uint32_t)) {
    throw std::out_of_range("write_idx too large");
  }

  union {
      uint32_t i;
      byte b[sizeof(uint32_t)];
  } u = {value};

  if (native_endianness() == Endianness::BigEndian) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  haystack[write_idx] = u.b[0];
  haystack[write_idx + 1] = u.b[1];
  haystack[write_idx + 2] = u.b[2];
  haystack[write_idx + 3] = u.b[3];

  return haystack;
}

/**
 * @brief Reads a @c uint64_t from @p haystack..
 *
 * @tparam N The array size in bytes.
 * @param haystack The array to read from.
 * @param read_idx The index to start reading at.
 * @return The @c uint64_t
 * @throws std::out_of_range If reading the @c uint64_t would cause a read outside of the array bounds.
 */
template<std::size_t N>
uint64_t read_le_uint64(std::array<byte, N> const &haystack, typename std::array<byte, N>::size_type read_idx) {
  if (read_idx > haystack.size() - sizeof(uint64_t)) {
    throw std::out_of_range("read_idx too large");
  }

  union {
      byte b[sizeof(uint64_t)];
      uint64_t i;
  } u = {{haystack[read_idx], haystack[read_idx + 1], haystack[read_idx + 2], haystack[read_idx + 3],
          haystack[read_idx + 4], haystack[read_idx + 5], haystack[read_idx + 6], haystack[read_idx + 7]}};

  if (native_endianness() == Endianness::BigEndian) {
    std::reverse(std::begin(u.b), std::end(u.b));
  }

  return u.i;
}

/**
 * @brief Writes a @c uint64_t @p haystack.
 *
 * @tparam N The array size in bytes.
 * @param haystack The array to write to.
 * @param write_idx The index to start writing at.
 * @param value The value to write.
 * @return A reference to @p haystack.
 * @throws std::out_of_range If writing the value to @p haystack would cause a write outside of the array bounds.
 */
template<std::size_t N>
std::array<byte, N>& write_le_uint64(std::array<byte, N> &haystack, typename std::array<byte, N>::size_type write_idx, uint64_t value) {
  if (write_idx > haystack.size() - sizeof(uint64_t)) {
    throw std::out_of_range("write_idx too large");
  }

  union {
      uint64_t i;
      byte b[sizeof(uint64_t)];
  } u = {value};

  if (native_endianness() == Endianness::BigEndian) {
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
 * @brief Calculates the FNV-1a hash of @p length message bytes starting at @p start_idx.
 *
 * @tparam N The message size in bytes.
 * @param message The message.
 * @param start_idx The index to start hashing from.
 * @param length The amount of bytes to be hashed.
 * @return The FNV-1a hash.
 */
template<std::size_t N>
uint32_t fnv1a(std::array<byte, N> const &message, typename std::array<byte, N>::size_type start_idx, typename std::array<byte, N>::size_type length) {
  if (length > N - start_idx) {
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

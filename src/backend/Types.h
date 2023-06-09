/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_TYPES_H_
#define NOID_SRC_BACKEND_TYPES_H_

#include <array>
#include <cstdint>
#include <vector>

namespace noid::backend {

using byte = unsigned char;
using PageNumber = std::uint32_t;
using Position = int64_t;

/**
 * @brief Convenience shorthand definition for no page.
 */
constexpr PageNumber NO_PAGE = 0;

constexpr int8_t C_API_ERROR = -1;

/**
 * @brief The default page size in bytes.
 */
constexpr uint16_t DEFAULT_PAGE_SIZE = 4096;

/**
 * @brief The default key size in bytes.
 * @details Currently changing the key size is not possible without incorrectly reading database files not having that
 * key size. The file format is equipped to handle dynamic key sizes, but the API is not at this time.
 */
constexpr uint8_t FIXED_KEY_SIZE = 16;

/**
 * @brief Alias for the search key type.
 */
using SearchKey = std::array<byte, FIXED_KEY_SIZE>;

/**
 * @brief Alias for the record value type.
 */
using V = std::vector<byte>;

/**
 * @brief Describes the available types of insert.
 */
enum class InsertType {

    /**
     * The data was inserted using a new slot within the node.
     */
    Insert,

    /**
     * Pre-existing data was overwritten on insert, and no new slot was used to store it.
     */
    Upsert,
};

}

#endif //NOID_SRC_BACKEND_TYPES_H_

/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_LIMITS_H_
#define NOID_SRC_BACKEND_PAGE_LIMITS_H_

#include <cstdint>

namespace noid::backend::page {

const uint8_t INTERNAL_NODE_HEADER_SIZE_BYTES = 24;
const uint8_t INTERNAL_NODE_ENTRY_SIZE_BYTES = 20;

const uint8_t LEAF_NODE_HEADER_SIZE_BYTES = 24;
const uint8_t LEAF_NODE_RECORD_SIZE_BYTES = 24;

/**
 * @brief Calculates the maximum amount of entries an internal node is allowed to contain, as
 * a function of page size.
 *
 * @param page_size The page size in bytes.
 * @return The maximum amount of entries for the given page size.
 */
inline constexpr uint8_t CalculateMaxEntries(uint16_t page_size) {
  return (uint8_t)((page_size - INTERNAL_NODE_HEADER_SIZE_BYTES) / INTERNAL_NODE_ENTRY_SIZE_BYTES);
}

/**
 * @brief Calculates the maximum amount of records a leaf node is allowed to contain, as
 * a function of page size.
 *
 * @param page_size The page size in bytes.
 * @return The maximum amount of records for the given page size.
 */
inline constexpr uint8_t CalculateMaxRecords(uint16_t page_size) {
  return (uint8_t)((page_size - LEAF_NODE_HEADER_SIZE_BYTES) / LEAF_NODE_RECORD_SIZE_BYTES);
}

}

#endif //NOID_SRC_BACKEND_PAGE_LIMITS_H_

/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_LIMITS_H_
#define NOID_SRC_BACKEND_PAGE_LIMITS_H_

#include <cstdint>

#include "backend/page/LeafNode.h"
#include "backend/page/NodeRecord.h"

namespace noid::backend::page {

/**
 * @brief The size in bytes of the @c InternalNode header.
 */
const uint8_t INTERNAL_NODE_HEADER_SIZE = 24;

/**
 * @brief The size in bytes of a @c NodeEntry within an @c InternalNode
 */
const uint8_t INTERNAL_NODE_ENTRY_SIZE = 20;

/**
 * @brief The size in bytes of a @c Freelist header.
 */
const uint8_t FREELIST_HEADER_SIZE = 12;

/**
 * @brief The size in bytes of a freelist entry.
 */
const uint8_t FREELIST_ENTRY_SIZE = sizeof(PageNumber);

/**
 * @brief Calculates the maximum amount of entries an internal node is allowed to contain, as
 * a function of page size.
 *
 * @param page_size The page size in bytes.
 * @return The maximum amount of entries for the given page size.
 */
inline uint16_t CalculateMaxEntries(uint16_t page_size)
{
  return (uint16_t) ((page_size - INTERNAL_NODE_HEADER_SIZE) / INTERNAL_NODE_ENTRY_SIZE);
}

/**
 * @brief Calculates the maximum amount of records a leaf node is allowed to contain, as
 * a function of page size.
 *
 * @param page_size The page size in bytes.
 * @return The maximum amount of records for the given page size.
 */
inline uint16_t CalculateMaxRecords(uint16_t page_size)
{
  return (uint16_t) ((page_size - page::LeafNode::HEADER_SIZE) / page::NodeRecord::SIZE);
}

/**
 * @brief Calculates the maximum amount of entries a freelist page is allowed to contain,
 * as a function of page size.
 *
 * @param page_size The page size in bytes.
 * @return The maximum amount of entries for the given page size.
 */
inline uint16_t CalculateMaxFreelistEntries(uint16_t page_size)
{
  return (uint16_t) ((page_size - FREELIST_HEADER_SIZE) / FREELIST_ENTRY_SIZE);
}

}

#endif //NOID_SRC_BACKEND_PAGE_LIMITS_H_

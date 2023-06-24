/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <cmath>
#include <stdexcept>

#include "NodeRecord.h"
#include "Overflow.h"
#include "backend/Bits.h"

namespace noid::backend::page {

NodeRecord::NodeRecord(std::array<byte, FIXED_KEY_SIZE> key, uint8_t inline_indicator,
    std::array<byte, INLINE_PAYLOAD_SIZE> payload)
    :key(key), inline_indicator(inline_indicator), payload(payload) { }

std::shared_ptr<NodeRecordBuilder> NodeRecord::NewBuilder()
{
  return std::shared_ptr<NodeRecordBuilder>(new NodeRecordBuilder());
}

std::shared_ptr<NodeRecordBuilder> NodeRecord::NewBuilder(const DynamicArray<byte>& container, DynamicArray<byte>::size_type read_idx)
{
  if (read_idx + NodeRecord::SIZE >= container.size()) {
    throw std::domain_error("Cannot read sizeof(NodeRecord) bytes from container[read_idx].");
  }

  auto builder = NodeRecord::NewBuilder();
  builder->key = read_container<byte, SearchKey>(container, read_idx, sizeof(SearchKey));
  builder->inline_payload_size = read_uint8<uint8_t>(container, read_idx + sizeof(SearchKey));
  builder->payload = read_container<byte, std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>>(container,
      read_idx + sizeof(SearchKey) + sizeof(uint8_t), NodeRecord::INLINE_PAYLOAD_SIZE);

  return builder;
}

uint32_t NodeRecord::CalculateOverflow(const V& value, const uint16_t page_size)
{
  if (value.size() <= NodeRecord::INLINE_PAYLOAD_SIZE) {
    return 0;
  }

  auto overflowed_bytes = value.size() - (NodeRecord::INLINE_PAYLOAD_SIZE - sizeof(PageNumber));
  return safe_cast<uint32_t>(std::ceil(static_cast<double>(overflowed_bytes) / (page_size - Overflow::HEADER_SIZE)));
}

const std::array<byte, FIXED_KEY_SIZE>& NodeRecord::GetKey() const
{
  return this->key;
}

uint8_t NodeRecord::GetInlineIndicator() const
{
  return this->inline_indicator;
}

const std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>& NodeRecord::GetPayload() const
{
  return this->payload;
}

PageNumber NodeRecord::GetOverflowPage() const
{
  if (this->inline_indicator != 0) {
    return NULL_PAGE;
  }

  return read_le_uint32<byte>(this->payload, this->payload.size() - sizeof(PageNumber));
}

/**** NodeRecordBuilder ****/

std::shared_ptr<NodeRecordBuilder> NodeRecordBuilder::WithSearchKey(SearchKey search_key)
{
  this->key = search_key;
  return this->shared_from_this();
}

std::shared_ptr<NodeRecordBuilder> NodeRecordBuilder::WithInlinePayload(std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE> data,
    uint8_t data_size)
{
  this->inline_payload_size = data_size;
  this->payload = data;

  return this->shared_from_this();
}

std::shared_ptr<NodeRecordBuilder> NodeRecordBuilder::WithOverflowPayload(std::array<byte, NodeRecord::OVERFLOW_PAYLOAD_SIZE> data,
    PageNumber first_overflow_page)
{
  this->inline_payload_size = 0;
  write_container<byte>(this->payload, 0, data);
  write_le_uint32<byte>(this->payload, data.size(), first_overflow_page);

  return this->shared_from_this();
}

NodeRecord NodeRecordBuilder::Build() const noexcept
{
  return {this->key, this->inline_payload_size, this->payload};
}

}
/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <algorithm>

#include "LeafNode.h"
#include "Limits.h"
#include "backend/Bits.h"
#include "backend/NoidConfig.h"

static uint16_t LEAF_NODE_MAGIC = 0x4e4c; // 'LN' for Leaf Node
static uint8_t MAGIC_OFFSET = 0;
static uint8_t RECORD_COUNT_OFFSET = 2;
static uint8_t LEFT_SIBLING_OFFSET = 5;
static uint8_t RIGHT_SIBLING_OFFSET = 9;

namespace noid::backend::page {

static DynamicArray<byte>& Validate(DynamicArray<byte>& data)
{
  auto page_size = NoidConfig::Get().vfs_page_size;
  if (data.size() == page_size && read_le_uint16<byte>(data, MAGIC_OFFSET) == LEAF_NODE_MAGIC
      && read_le_uint16<byte>(data, RECORD_COUNT_OFFSET) < CalculateMaxRecords(page_size)) {
    return data;
  }

  throw std::invalid_argument("Given raw data is not a valid serialized LeafNode instance.");
}

static inline std::size_t SlotToIndex(uint16_t slot)
{
  return RECORD_COUNT_OFFSET + slot * sizeof(PageNumber);
}

NodeRecord::NodeRecord(std::array<byte, FIXED_KEY_SIZE> key, byte inline_indicator,
    std::array<byte, INLINE_PAYLOAD_SIZE> payload)
    :key(key), inline_indicator(inline_indicator), payload(payload) { }

PageNumber NodeRecord::GetOverflowPage() const
{
  if (this->inline_indicator != 0) {
    throw std::domain_error("Inline indicator set: no overflow page exists.");
  }

  return read_le_uint32<byte>(this->payload, this->payload.size() - sizeof(PageNumber));
}

const std::array<byte, FIXED_KEY_SIZE>& NodeRecord::GetKey() const
{
  return this->key;
}

byte NodeRecord::GetInlineIndicator() const
{
  return this->inline_indicator;
}

const std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>& NodeRecord::GetPayload() const
{
  return this->payload;
}

bool NodeRecord::operator==(const NodeRecord& other) const
{
  return this->key == other.key && this->inline_indicator == other.inline_indicator && this->payload == other.payload;
}

LeafNode::LeafNode(PageNumber left_sibling, PageNumber right_sibling, DynamicArray<NodeRecord>&& records)
    :left_sibling(left_sibling), right_sibling(right_sibling), records(std::move(records)) { }

std::unique_ptr<LeafNodeBuilder> LeafNode::NewBuilder()
{
  return std::unique_ptr<LeafNodeBuilder>(new LeafNodeBuilder());
}

std::unique_ptr<LeafNodeBuilder> LeafNode::NewBuilder(const LeafNode& base)
{
  return std::unique_ptr<LeafNodeBuilder>(new LeafNodeBuilder(base));
}

std::unique_ptr<LeafNodeBuilder> LeafNode::NewBuilder(DynamicArray<byte>&& base)
{
  return std::unique_ptr<LeafNodeBuilder>(new LeafNodeBuilder(std::move(Validate(base))));
}

uint16_t LeafNode::Size() const
{
  return static_cast<uint16_t>(this->records.size());
}

PageNumber LeafNode::GetLeftSibling() const
{
  return this->left_sibling;
}

PageNumber LeafNode::GetRightSibling() const
{
  return this->right_sibling;
}

const NodeRecord& LeafNode::RecordAt(uint16_t slot) const
{
  return const_cast<const NodeRecord&>(this->records.at(slot));
}

/*** LeafNodeBuilder ***/

LeafNodeBuilder::LeafNodeBuilder()
    :page_size(NoidConfig::Get().vfs_page_size), max_slot(CalculateMaxRecords(page_size) - 1), left_sibling(0),
     right_sibling(0)
{
  this->records.reserve(this->max_slot + 1);
}

LeafNodeBuilder::LeafNodeBuilder(const LeafNode& base)
    :page_size(NoidConfig::Get().vfs_page_size), max_slot(CalculateMaxRecords(page_size) - 1),
     left_sibling(base.left_sibling), right_sibling(base.right_sibling), records(base.records.size()) {
  std::copy(base.records.begin(), base.records.end(), this->records.begin());
}

LeafNodeBuilder::LeafNodeBuilder(DynamicArray<byte>&& base)
    :page_size(NoidConfig::Get().vfs_page_size), max_slot(CalculateMaxRecords(page_size) - 1),
     left_sibling(read_le_uint16<byte>(base, LEFT_SIBLING_OFFSET)),
     right_sibling(read_le_uint16<byte>(base, RIGHT_SIBLING_OFFSET))
{
  auto next_free_slot = read_le_uint16<byte>(base, RECORD_COUNT_OFFSET);
  this->records.reserve(next_free_slot);

  for (auto slot = 0; slot < next_free_slot; slot++) {
    auto record_offset = SlotToIndex(slot);
    this->records.emplace_back(
        read_container<byte, std::array<byte, FIXED_KEY_SIZE>>(base, record_offset, FIXED_KEY_SIZE),
        read_uint8<byte>(base, record_offset + FIXED_KEY_SIZE),
        read_container<byte, std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>>(base,
            record_offset + FIXED_KEY_SIZE + sizeof(uint8_t), NodeRecord::INLINE_PAYLOAD_SIZE)
    );
  }
}

std::unique_ptr<const LeafNode> LeafNodeBuilder::Build()
{
  auto record_list = DynamicArray<NodeRecord>(this->records.size());
  std::copy(this->records.begin(), this->records.end(), record_list.begin());

  return std::unique_ptr<const LeafNode>(
      new LeafNode(this->left_sibling, this->right_sibling, std::move(record_list)));
}

bool LeafNodeBuilder::IsFull()
{
  return this->records.size() - 1 == max_slot;
}

LeafNodeBuilder& LeafNodeBuilder::WithLeftSibling(PageNumber sibling)
{
  this->left_sibling = sibling;

  return *this;
}

LeafNodeBuilder& LeafNodeBuilder::WithRightSibling(PageNumber sibling)
{
  this->right_sibling = sibling;

  return *this;
}

LeafNodeBuilder& LeafNodeBuilder::WithRecord(NodeRecord record)
{
  if (this->IsFull()) {
    throw std::overflow_error("Cannot add record: node is full");
  }

  this->records.push_back(record);

  return *this;
}

LeafNodeBuilder& LeafNodeBuilder::WithRecord(NodeRecord record, uint16_t slot_hint)
{
  if (this->records.size() > slot_hint) {
    this->records[slot_hint] = record;
    return *this;
  }

  return this->WithRecord(record);
}

}
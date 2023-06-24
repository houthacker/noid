/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <algorithm>

#include "LeafNode.h"
#include "Limits.h"
#include "NodeRecord.h"
#include "backend/Algorithm.h"
#include "backend/Bits.h"
#include "backend/page/Overflow.h"

static const uint16_t LEAF_NODE_MAGIC = 0x4e4c; // 'LN' for Leaf Node
static const uint8_t MAGIC_OFFSET = 0;
static const uint8_t RECORD_COUNT_OFFSET = 2;
static const uint8_t LEFT_SIBLING_OFFSET = 4;
static const uint8_t RIGHT_SIBLING_OFFSET = 8;

namespace noid::backend::page {

static DynamicArray<byte>& Validate(DynamicArray<byte>& data, uint16_t page_size)
{
  if (data.size() == page_size
      && read_le_uint16<byte>(data, MAGIC_OFFSET) == LEAF_NODE_MAGIC
      && read_le_uint16<byte>(data, RECORD_COUNT_OFFSET) < CalculateMaxRecords(page_size)) {
    return data;
  }

  throw std::invalid_argument("Given raw data is not a valid serialized LeafNode instance.");
}

static inline std::size_t SlotToIndex(uint16_t slot)
{
  return RECORD_COUNT_OFFSET + slot * sizeof(PageNumber);
}

LeafNode::LeafNode(uint16_t next_record_slot, PageNumber left_sibling, PageNumber right_sibling,
    DynamicArray<NodeRecord>&& records, uint16_t page_size)
    :next_record_slot(next_record_slot), left_sibling(left_sibling), right_sibling(right_sibling),
     records(std::move(records)), page_size(page_size) { }

std::shared_ptr<LeafNodeBuilder> LeafNode::NewBuilder(uint16_t page_size)
{
  return std::shared_ptr<LeafNodeBuilder>(new LeafNodeBuilder(page_size));
}

std::shared_ptr<LeafNodeBuilder> LeafNode::NewBuilder(const LeafNode& base)
{
  return std::shared_ptr<LeafNodeBuilder>(new LeafNodeBuilder(base));
}

std::shared_ptr<LeafNodeBuilder> LeafNode::NewBuilder(DynamicArray<byte>&& base)
{
  return std::shared_ptr<LeafNodeBuilder>(new LeafNodeBuilder(std::move(Validate(base, base.size()))));
}

uint16_t LeafNode::Size() const
{
  return this->next_record_slot; //abc
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

bool LeafNode::Contains(const SearchKey& key) const
{
  return BinarySearch<NodeRecord>(this->records, 0, this->next_record_slot, key) != this->records.size();
}

DynamicArray<byte> LeafNode::ToBytes() const
{
  auto bytes = DynamicArray<byte>(this->page_size);
  write_le_uint16<byte>(bytes, MAGIC_OFFSET, LEAF_NODE_MAGIC);

  return bytes;
}

/*** LeafNodeBuilder ***/

LeafNodeBuilder::LeafNodeBuilder(uint16_t page_size)
    :page_size(page_size), max_record_slot(CalculateMaxRecords(page_size) - 1), left_sibling(0), right_sibling(0)
{
  this->records.reserve(max_record_slot + 1);
}

LeafNodeBuilder::LeafNodeBuilder(const LeafNode& base)
    :page_size(base.page_size), max_record_slot(CalculateMaxRecords(page_size) - 1),
     left_sibling(base.left_sibling), right_sibling(base.right_sibling), records(base.next_record_slot)
{
  std::ranges::copy(base.records.begin(), base.records.begin() + base.next_record_slot, this->records.begin());
}

LeafNodeBuilder::LeafNodeBuilder(DynamicArray<byte>&& base)
    :page_size(safe_cast<uint16_t>(base.size())),
     max_record_slot(CalculateMaxRecords(safe_cast<uint16_t>(base.size())) - 1),
     left_sibling(read_le_uint16<byte>(base, LEFT_SIBLING_OFFSET)),
     right_sibling(read_le_uint16<byte>(base, RIGHT_SIBLING_OFFSET))
{
  auto next_free_slot = read_le_uint16<byte>(base, RECORD_COUNT_OFFSET);
  this->records.reserve(next_free_slot);

  for (uint16_t slot = 0; slot < next_free_slot; slot++) {
    this->records.push_back(NodeRecord::NewBuilder(base, SlotToIndex(slot))->Build());
  }
}

std::unique_ptr<const LeafNode> LeafNodeBuilder::Build()
{
  auto record_list = DynamicArray<NodeRecord>(this->max_record_slot + 1);
  std::ranges::copy(this->records.begin(), this->records.end(), record_list.begin());

  return std::unique_ptr<const LeafNode>(
      new LeafNode(safe_cast<uint16_t>(this->records.size()), this->left_sibling, this->right_sibling,
          std::move(record_list), this->page_size));
}

bool LeafNodeBuilder::IsFull() const
{
  return this->records.size() == safe_cast<decltype(max_record_slot)>(max_record_slot + 1);
}

std::shared_ptr<LeafNodeBuilder> LeafNodeBuilder::WithLeftSibling(PageNumber sibling)
{
  this->left_sibling = sibling;

  return this->shared_from_this();
}

std::shared_ptr<LeafNodeBuilder> LeafNodeBuilder::WithRightSibling(PageNumber sibling)
{
  this->right_sibling = sibling;

  return this->shared_from_this();
}

std::shared_ptr<LeafNodeBuilder> LeafNodeBuilder::WithRecord(NodeRecord record)
{
  if (this->IsFull()) {
    throw std::overflow_error("Cannot add record: node is full");
  }

  this->records.push_back(record);

  return this->shared_from_this();
}

std::shared_ptr<LeafNodeBuilder> LeafNodeBuilder::WithRecord(NodeRecord record, uint16_t slot_hint)
{
  if (this->records.size() > slot_hint) {
    this->records[slot_hint] = record;
    return this->shared_from_this();
  }

  return this->WithRecord(record);
}

}
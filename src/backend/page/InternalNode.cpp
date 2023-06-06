/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <stdexcept>

#include "InternalNode.h"
#include "Limits.h"
#include "backend/Bits.h"
#include "backend/NoidConfig.h"

static uint16_t INTERNAL_NODE_MAGIC = 0x4e49; // 'IN' for Internal Node
static uint8_t MAGIC_OFFSET = 0;
static uint8_t ENTRY_COUNT_OFFSET = 2;
static uint8_t LEFTMOST_CHILD_PAGE_OFFSET = 8;
static uint8_t ENTRY_LIST_OFFSET = 24;

namespace noid::backend::page {

static std::vector<byte>& Validate(std::vector<byte>& raw_data)
{
  auto page_size = NoidConfig::Get().vfs_page_size;
  if (raw_data.size() == page_size
      && read_le_uint16<byte>(raw_data, MAGIC_OFFSET) == INTERNAL_NODE_MAGIC
      && read_le_uint16<byte>(raw_data, ENTRY_COUNT_OFFSET) <= CalculateMaxEntries(page_size)) {
    return raw_data;
  }

  throw std::invalid_argument("Given raw data is not a valid serialized InternalNode instance.");
}

static inline std::size_t SlotToIndex(uint16_t slot)
{
  return ENTRY_LIST_OFFSET + slot * sizeof(PageNumber);
}

bool NodeEntry::operator==(const NodeEntry& other) const
{
  return this->right_child == other.right_child && this->key == other.key;
}

InternalNode::InternalNode(PageNumber leftmost_child, std::vector<NodeEntry>&& entries)
    :leftmost_child(leftmost_child), entries(std::move(entries)) { }

std::unique_ptr<InternalNodeBuilder> InternalNode::NewBuilder()
{
  return std::unique_ptr<InternalNodeBuilder>(new InternalNodeBuilder());
}

std::unique_ptr<InternalNodeBuilder> InternalNode::NewBuilder(const InternalNode& base)
{
  return std::unique_ptr<InternalNodeBuilder>(new InternalNodeBuilder(base));
}

std::unique_ptr<InternalNodeBuilder> InternalNode::NewBuilder(std::vector<byte>&& base)
{
  return std::unique_ptr<InternalNodeBuilder>(new InternalNodeBuilder(std::move(Validate(base))));
}

uint16_t InternalNode::Size() const
{
  return static_cast<uint16_t>(this->entries.size());
}

PageNumber InternalNode::GetLeftmostChild() const
{
  return this->leftmost_child;
}

const NodeEntry& InternalNode::EntryAt(uint16_t slot) const
{
  return const_cast<NodeEntry&>(this->entries.at(slot));
}

/*** InternalNodeBuilder ***/

InternalNodeBuilder::InternalNodeBuilder()
    :page_size(NoidConfig::Get().vfs_page_size), max_slot(CalculateMaxEntries(page_size) - 1), leftmost_child(0)
{
  this->entries.reserve(max_slot + 1);
}

InternalNodeBuilder::InternalNodeBuilder(const InternalNode& base)
    :page_size(NoidConfig::Get().vfs_page_size), max_slot(CalculateMaxEntries(page_size) - 1),
     leftmost_child(base.leftmost_child), entries(base.entries) { }

InternalNodeBuilder::InternalNodeBuilder(std::vector<byte>&& base)
    :page_size(NoidConfig::Get().vfs_page_size), max_slot(CalculateMaxEntries(page_size) - 1),
     leftmost_child(read_le_uint32<byte>(base, LEFTMOST_CHILD_PAGE_OFFSET))
{
  auto next_free_slot = read_le_uint16<byte>(base, ENTRY_COUNT_OFFSET);
  this->entries.reserve(next_free_slot);

  for (auto slot = 0; slot < next_free_slot; slot++) {
    auto container_offset = SlotToIndex(slot);
    this->entries.push_back({
        read_container<byte, std::array<byte, FIXED_KEY_SIZE>>(base, container_offset, FIXED_KEY_SIZE),
        read_le_uint32<byte>(base, container_offset + FIXED_KEY_SIZE)
    });
  }
}

std::unique_ptr<const InternalNode> InternalNodeBuilder::Build()
{
  return std::unique_ptr<const InternalNode>(new InternalNode(this->leftmost_child, std::move(this->entries)));
}

bool InternalNodeBuilder::IsFull()
{
  return this->entries.size() - 1 == this->max_slot;
}

InternalNodeBuilder& InternalNodeBuilder::WithLeftmostChild(PageNumber page_number)
{
  this->leftmost_child = page_number;

  return *this;
}

InternalNodeBuilder& InternalNodeBuilder::WithEntry(std::array<byte, FIXED_KEY_SIZE> key, PageNumber right_child_page)
{
  if (this->IsFull()) {
    throw std::overflow_error("Cannot add entry: node is full");
  }

  this->entries.push_back({key, right_child_page});

  return *this;
}

InternalNodeBuilder& InternalNodeBuilder::WithEntry(std::array<byte, FIXED_KEY_SIZE> key, PageNumber right_child_page,
    uint16_t slot_hint)
{
  if (this->entries.size() > slot_hint) {
    // slot_hint refers to an occupied slot, so replace it.
    this->entries[slot_hint] = {key, right_child_page};
    return *this;
  }

  // otherwise, append the entry to the end of the list.
  return this->WithEntry(key, right_child_page);
}

}

/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <algorithm>
#include <stdexcept>

#include "InternalNode.h"
#include "Limits.h"
#include "backend/Algorithm.h"
#include "backend/Bits.h"

static const uint16_t INTERNAL_NODE_MAGIC = 0x4e49; // 'IN' for Internal Node
static const uint8_t MAGIC_OFFSET = 0;
static const uint8_t ENTRY_COUNT_OFFSET = 2;
static const uint8_t LEFTMOST_CHILD_PAGE_OFFSET = 4;

namespace noid::backend::page {

static DynamicArray<byte>& Validate(DynamicArray<byte>& raw_data, uint16_t page_size)
{
  if (raw_data.size() == page_size
      && read_le_uint16<byte>(raw_data, MAGIC_OFFSET) == INTERNAL_NODE_MAGIC
      && read_le_uint16<byte>(raw_data, ENTRY_COUNT_OFFSET) <= CalculateMaxEntries(page_size)) {
    return raw_data;
  }

  throw std::invalid_argument("Given raw data is not a valid serialized InternalNode instance.");
}

static inline std::size_t SlotToIndex(uint16_t slot)
{
  return InternalNode::HEADER_SIZE + slot * sizeof(PageNumber);
}

const SearchKey& NodeEntry::GetKey() const
{
  return this->key;
}

bool NodeEntry::operator==(const NodeEntry& other) const
{
  return this->right_child == other.right_child && this->key == other.key;
}

InternalNode::InternalNode(PageNumber location, uint16_t next_entry_slot, PageNumber leftmost_child,
    DynamicArray<NodeEntry>&& entries, uint16_t page_size)
    :location(location), next_entry_slot(next_entry_slot), leftmost_child(leftmost_child), entries(std::move(entries)),
     page_size(page_size) { }

std::shared_ptr<InternalNodeBuilder> InternalNode::NewBuilder(uint16_t page_size)
{
  return std::shared_ptr<InternalNodeBuilder>(new InternalNodeBuilder(page_size));
}

std::shared_ptr<InternalNodeBuilder> InternalNode::NewBuilder(const InternalNode& base)
{
  return std::shared_ptr<InternalNodeBuilder>(new InternalNodeBuilder(base));
}

std::shared_ptr<InternalNodeBuilder> InternalNode::NewBuilder(DynamicArray<byte>&& base)
{
  return std::shared_ptr<InternalNodeBuilder>(new InternalNodeBuilder(std::move(Validate(base, base.size()))));
}

PageNumber InternalNode::GetLocation() const
{
  return this->location;
}

uint16_t InternalNode::Size() const
{
  return this->next_entry_slot;
}

PageNumber InternalNode::GetLeftmostChild() const
{
  return this->leftmost_child;
}

const NodeEntry& InternalNode::EntryAt(uint16_t slot) const
{
  if (slot >= this->next_entry_slot) {
    throw std::out_of_range("slot");
  }

  return this->entries[slot];
}

bool InternalNode::Contains(const SearchKey& key) const
{
  return BinarySearch<NodeEntry>(this->entries, 0, this->next_entry_slot, key)
      != this->entries.size();
}

DynamicArray<byte> InternalNode::ToBytes() const
{
  auto bytes = DynamicArray<byte>(this->page_size);
  write_le_uint16<byte>(bytes, MAGIC_OFFSET, INTERNAL_NODE_MAGIC);
  write_le_uint16<byte>(bytes, ENTRY_COUNT_OFFSET, this->next_entry_slot);
  write_le_uint32<byte>(bytes, LEFTMOST_CHILD_PAGE_OFFSET, this->leftmost_child);
  write_contiguous_container<NodeEntry>(bytes, InternalNode::HEADER_SIZE, this->entries, 0, this->next_entry_slot);

  return bytes;
}

/*** InternalNodeBuilder ***/

InternalNodeBuilder::InternalNodeBuilder(uint16_t page_size)
    :page_size(page_size), max_entry_slot(CalculateMaxEntries(page_size) - 1), leftmost_child(0)
{
  this->entries.reserve(max_entry_slot + 1);
}

InternalNodeBuilder::InternalNodeBuilder(const InternalNode& base)
    :location(base.location), page_size(base.page_size), max_entry_slot(CalculateMaxEntries(page_size) - 1),
     leftmost_child(base.leftmost_child), entries(base.next_entry_slot)
{
  std::ranges::copy(base.entries.begin(), base.entries.begin() + base.next_entry_slot, this->entries.begin());
}

InternalNodeBuilder::InternalNodeBuilder(DynamicArray<byte>&& base)
    :page_size(safe_cast<uint16_t>(base.size())), max_entry_slot(CalculateMaxEntries(page_size) - 1),
     leftmost_child(read_le_uint32<byte>(base, LEFTMOST_CHILD_PAGE_OFFSET))
{
  auto next_free_slot = read_le_uint16<byte>(base, ENTRY_COUNT_OFFSET);
  this->entries.reserve(next_free_slot);

  for (uint16_t slot = 0; slot < next_free_slot; slot++) {
    auto entry_offset = SlotToIndex(slot);
    this->entries.push_back({
        read_container<byte, SearchKey>(base, entry_offset, FIXED_KEY_SIZE),
        read_le_uint32<byte>(base, entry_offset + FIXED_KEY_SIZE)
    });
  }
}

std::unique_ptr<const InternalNode> InternalNodeBuilder::Build()
{
  if (this->location == NULL_PAGE) {
    throw std::domain_error("Cannot build InternalNode: location not set.");
  }

  auto entry_list = DynamicArray<NodeEntry>(CalculateMaxEntries(this->page_size));
  std::ranges::copy(this->entries.begin(), this->entries.end(), entry_list.begin());

  return std::unique_ptr<const InternalNode>(
      new InternalNode(this->location, safe_cast<uint16_t>(this->entries.size()), this->leftmost_child, std::move(entry_list),
          this->page_size));
}

bool InternalNodeBuilder::IsFull() const
{
  return this->entries.size() - 1 == this->max_entry_slot;
}

std::shared_ptr<InternalNodeBuilder> InternalNodeBuilder::WithLocation(PageNumber loc)
{
  if (loc == NULL_PAGE) {
    throw std::domain_error("GetLocation cannot be zero.");
  }

  this->location = loc;
  return this->shared_from_this();
}

std::shared_ptr<InternalNodeBuilder> InternalNodeBuilder::WithLeftmostChild(PageNumber page_number)
{
  this->leftmost_child = page_number;

  return this->shared_from_this();
}

std::shared_ptr<InternalNodeBuilder> InternalNodeBuilder::WithEntry(SearchKey key, PageNumber right_child_page)
{
  if (this->IsFull()) {
    throw std::overflow_error("Cannot add entry: node is full");
  }

  this->entries.push_back({key, right_child_page});

  return this->shared_from_this();
}

std::shared_ptr<InternalNodeBuilder> InternalNodeBuilder::WithEntry(SearchKey key, PageNumber right_child_page,
    uint16_t slot_hint)
{
  if (this->entries.size() > slot_hint) {
    // slot_hint refers to an occupied slot, so replace it.
    this->entries[slot_hint] = {key, right_child_page};
    return this->shared_from_this();
  }

  // otherwise, append the entry to the end of the list.
  return this->WithEntry(key, right_child_page);
}

}

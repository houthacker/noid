/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <stdexcept>

#include "InternalNode.h"
#include "Limits.h"
#include "backend/Bits.h"
#include "backend/NoidConfig.h"

static uint16_t INTERNAL_NODE_MAGIC = 0x5049;
static uint8_t MAGIC_OFFSET = 0;
static uint8_t ENTRY_COUNT_OFFSET = 2;
static uint8_t LEFTMOST_CHILD_PAGE_OFFSET = 3;
static uint8_t ENTRY_LIST_OFFSET = 24;

namespace noid::backend::page {

static std::vector<byte>& Validate(std::vector<byte>& raw_data) {
  auto page_size = NoidConfig::Get().vfs_page_size;
  if (raw_data.size() == page_size
      && read_le_uint16<byte>(raw_data, MAGIC_OFFSET) == INTERNAL_NODE_MAGIC
      && read_uint8<byte>(raw_data, ENTRY_COUNT_OFFSET) <= CalculateMaxEntries(page_size)) {
    return raw_data;
  }

  throw std::invalid_argument("Given raw data is not a valid serialized InternalNode instance.");
}

InternalNode::InternalNode(std::vector<byte> &&data) : data(std::move(data)) {}

std::unique_ptr<InternalNodeBuilder> InternalNode::NewBuilder() {
  return std::unique_ptr<InternalNodeBuilder>(new InternalNodeBuilder());
}

std::unique_ptr<InternalNodeBuilder> InternalNode::NewBuilder(const InternalNode &base) {
  return std::unique_ptr<InternalNodeBuilder>(new InternalNodeBuilder(base));
}

std::unique_ptr<InternalNodeBuilder> InternalNode::NewBuilder(std::vector<byte> &&base) {
  return std::unique_ptr<InternalNodeBuilder>(new InternalNodeBuilder(std::move(Validate(base))));
}

uint8_t InternalNode::GetEntryCount() const {
  return read_uint8<byte>(this->data, ENTRY_COUNT_OFFSET);
}

PageNumber InternalNode::GetLeftmostChild() const {
  return read_le_uint32<byte>(this->data, LEFTMOST_CHILD_PAGE_OFFSET);
}

NodeEntry InternalNode::EntryAt(uint8_t pos) const {
  uint32_t offset = ENTRY_LIST_OFFSET + (pos * INTERNAL_NODE_ENTRY_SIZE_BYTES);
  if (offset >= this->GetEntryCount()) {
    throw std::out_of_range("No such entry");
  }

  return NodeEntry{
    read_container<byte, std::array<byte, FIXED_KEY_SIZE>>(this->data, offset, FIXED_KEY_SIZE),
read_le_uint32<byte>(this->data, offset + FIXED_KEY_SIZE)
  };
}

/*** InternalNodeBuilder ***/


InternalNodeBuilder::InternalNodeBuilder() :
    page_size(NoidConfig::Get().vfs_page_size), data(page_size) {
  write_le_uint16<byte>(this->data, MAGIC_OFFSET, INTERNAL_NODE_MAGIC);
}

InternalNodeBuilder::InternalNodeBuilder(const InternalNode &base) :
   page_size(NoidConfig::Get().vfs_page_size), data(base.data) {}

InternalNodeBuilder::InternalNodeBuilder(std::vector<byte> &&base) :
    page_size(NoidConfig::Get().vfs_page_size), data(std::move(base)){}

std::unique_ptr<const InternalNode> InternalNodeBuilder::Build() {
  return std::unique_ptr<const InternalNode>(new InternalNode(std::move(this->data)));
}

bool InternalNodeBuilder::IsFull() {
  return read_uint8<byte>(this->data, ENTRY_COUNT_OFFSET) == CalculateMaxEntries(this->page_size);
}

InternalNodeBuilder &InternalNodeBuilder::WithLeftmostChildPage(PageNumber page_number) {
  write_le_uint32<byte>(this->data, LEFTMOST_CHILD_PAGE_OFFSET, page_number);

  return *this;
}

InternalNodeBuilder &InternalNodeBuilder::WithEntry(std::array<byte, FIXED_KEY_SIZE> key, PageNumber right_child_page) {
  auto current_entry_count = read_uint8<byte>(this->data, ENTRY_COUNT_OFFSET);
  if (this->IsFull()) {
    throw std::overflow_error("Cannot add entry: node is full");
  }

  // Write the entry into the data
  auto entry_write_offset = ENTRY_LIST_OFFSET + (current_entry_count * INTERNAL_NODE_ENTRY_SIZE_BYTES);
  write_container<byte>(this->data, entry_write_offset, key);
  write_le_uint32<byte>(this->data, entry_write_offset + FIXED_KEY_SIZE, right_child_page);

  // Increment the entry count
  write_uint8<byte>(this->data, ENTRY_COUNT_OFFSET, current_entry_count + 1);

  return *this;
}

}

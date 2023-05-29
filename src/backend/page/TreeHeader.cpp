/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "TreeHeader.h"
#include "backend/Bits.h"
#include "backend/NoidConfig.h"

static uint8_t TREE_HEADER_MAGIC_OFFSET = 0;
static uint8_t MAX_ENTRIES_OFFSET = 2;
static uint8_t MAX_RECORDS_OFFSET = 3;
static uint8_t ROOT_NODE_PAGE_OFFSET = 4;

namespace noid::backend::page {

static inline uint8_t MaxEntries(uint16_t page_size) {
  // TODO replace 24 by InternalNode::HEADER_SIZE_BYTES
  // TODO replace 20 by InternalNodeEntry::SIZE_BYTES
  return (uint8_t)((page_size - 24) / 20);
}

static inline uint8_t MaxRecords(uint16_t page_size) {
  // TODO replace 24 by LeafNode::HEADER_SIZE_BYTES
  // TODO replace 24 by LeafNodeRecord::SIZE_BYTES
  return (uint8_t)((page_size - 24) / 24);
}

static std::vector<byte>& Validate(std::vector<byte>& data) {
  auto magic = read_le_uint16<byte>(data, TREE_HEADER_MAGIC_OFFSET);
  if (magic != (uint16_t)TreeType::Index && magic != (uint16_t)TreeType::Table) {
    throw std::invalid_argument("Invalid tree header page magic.");
  }

  auto page_size = NoidConfig::Get().vfs_page_size;
  auto max_entries_configured = MaxEntries(page_size);
  if (read_uint8<byte>(data, MAX_ENTRIES_OFFSET) != max_entries_configured) {
    throw std::invalid_argument("Unsupported max internal node entries");
  }

  auto max_records_configured = MaxRecords(page_size);
  if (read_uint8<byte>(data, MAX_RECORDS_OFFSET) != max_records_configured) {
    throw std::invalid_argument("Unsupported max leaf node records");
  }

  return data;
}

TreeHeader::TreeHeader(std::vector<byte> && data) : data(std::move(data)) {}

std::unique_ptr<TreeHeaderBuilder> TreeHeader::NewBuilder() {
  return std::unique_ptr<TreeHeaderBuilder>(new TreeHeaderBuilder());
}

std::unique_ptr<TreeHeaderBuilder> TreeHeader::NewBuilder(const TreeHeader &base) {
  return std::unique_ptr<TreeHeaderBuilder>(new TreeHeaderBuilder(base));
}

std::unique_ptr<TreeHeaderBuilder> TreeHeader::NewBuilder(std::vector<byte> &&base) {
  return std::unique_ptr<TreeHeaderBuilder>(new TreeHeaderBuilder(std::move(Validate(base))));
}

TreeType TreeHeader::GetTreeType() const {
  return static_cast<TreeType>(read_le_uint16<byte>(this->data, TREE_HEADER_MAGIC_OFFSET));
}

uint8_t TreeHeader::GetMaxInternalEntries() const {
  return read_uint8<byte>(this->data, MAX_ENTRIES_OFFSET);
}

uint8_t TreeHeader::GetMaxLeafRecords() const {
  return read_uint8<byte>(this->data, MAX_RECORDS_OFFSET);
}

PageNumber TreeHeader::GetRootNodePageNumber() const {
  return read_le_uint32<byte>(this->data, ROOT_NODE_PAGE_OFFSET);
}

/*** TreeHeaderBuilder ***/

TreeHeaderBuilder::TreeHeaderBuilder() :
    page_size(NoidConfig::Get().vfs_page_size),
    data(this->page_size) {
  this->max_entries = MaxEntries(this->page_size);
  this->max_records = MaxRecords(this->page_size);
}

TreeHeaderBuilder::TreeHeaderBuilder(const TreeHeader &base) :
    page_size(NoidConfig::Get().vfs_page_size),
    max_entries(base.GetMaxInternalEntries()),
    max_records(base.GetMaxInternalEntries()),
    data(base.data) {}

TreeHeaderBuilder::TreeHeaderBuilder(std::vector<byte> &&base) : page_size(NoidConfig::Get().vfs_page_size) {
  this->max_entries = read_uint8<byte>(base, MAX_ENTRIES_OFFSET);
  this->max_records = read_uint8<byte>(base, MAX_RECORDS_OFFSET);
  this->data = std::move(base);
}

std::unique_ptr<const TreeHeader> TreeHeaderBuilder::Build() {
  if (read_le_uint16<byte>(this->data, TREE_HEADER_MAGIC_OFFSET) == 0x0000) {
    throw std::domain_error("Tree type of TreeHeader not set.");
  }

  return std::unique_ptr<const TreeHeader>(new TreeHeader(std::move(this->data)));
}

TreeHeaderBuilder &TreeHeaderBuilder::WithTreeType(TreeType tree_type) {
  auto magic = read_le_uint16<byte>(this->data, TREE_HEADER_MAGIC_OFFSET);
  if (magic != 0x0000 && magic != (uint16_t)tree_type) {
    throw std::domain_error("Tree header magic already set to incompatible type");
  }

  write_le_uint16<byte>(this->data, TREE_HEADER_MAGIC_OFFSET, (uint16_t)tree_type);
  return *this;
}

TreeHeaderBuilder &TreeHeaderBuilder::WithRootPageNumber(PageNumber root_page) {
  write_le_uint32<byte>(this->data, ROOT_NODE_PAGE_OFFSET, root_page);

  return *this;
}

}
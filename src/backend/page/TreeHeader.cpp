/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "TreeHeader.h"
#include "Limits.h"
#include "backend/Bits.h"
#include "backend/NoidConfig.h"

static const uint8_t TREE_HEADER_MAGIC_OFFSET = 0;
static const uint8_t MAX_ENTRIES_OFFSET = 2;
static const uint8_t MAX_RECORDS_OFFSET = 4;
static const uint8_t ROOT_NODE_PAGE_OFFSET = 6;
static const uint8_t PAGE_COUNT_OFFSET = 10;

namespace noid::backend::page {

static DynamicArray<byte>& Validate(DynamicArray<byte>& data)
{
  if (auto magic = read_le_uint16<byte>(data, TREE_HEADER_MAGIC_OFFSET); magic != (uint16_t) TreeType::Index
      && magic != (uint16_t) TreeType::Table) {
    throw std::invalid_argument("Invalid tree header page magic.");
  }

  auto page_size = NoidConfig::Get().vfs_page_size;
  if (read_le_uint16<byte>(data, MAX_ENTRIES_OFFSET) != CalculateMaxEntries(page_size)) {
    throw std::invalid_argument("Unsupported max internal node entries");
  }

  if (read_le_uint16<byte>(data, MAX_RECORDS_OFFSET) != CalculateMaxRecords(page_size)) {
    throw std::invalid_argument("Unsupported max leaf node records");
  }

  return data;
}

TreeHeader::TreeHeader(TreeType tree_type, uint16_t max_node_entries, uint16_t max_node_records, PageNumber root,
    uint32_t page_count, uint16_t page_size)
    :tree_type(tree_type), max_node_entries(max_node_entries), max_node_records(max_node_records), root(root),
     page_count(page_count), page_size(page_size) { }

std::unique_ptr<TreeHeaderBuilder> TreeHeader::NewBuilder()
{
  return std::unique_ptr<TreeHeaderBuilder>(new TreeHeaderBuilder());
}

std::unique_ptr<TreeHeaderBuilder> TreeHeader::NewBuilder(const TreeHeader& base)
{
  return std::unique_ptr<TreeHeaderBuilder>(new TreeHeaderBuilder(base));
}

std::unique_ptr<TreeHeaderBuilder> TreeHeader::NewBuilder(DynamicArray<byte>&& base)
{
  return std::unique_ptr<TreeHeaderBuilder>(new TreeHeaderBuilder(std::move(Validate(base))));
}

TreeType TreeHeader::GetTreeType() const
{
  return this->tree_type;
}

uint16_t TreeHeader::GetMaxInternalEntries() const
{
  return this->max_node_entries;
}

uint16_t TreeHeader::GetMaxLeafRecords() const
{
  return this->max_node_records;
}

PageNumber TreeHeader::GetRoot() const
{
  return this->root;
}

uint32_t TreeHeader::GetPageCount() const
{
  return this->page_count;
}

DynamicArray<byte> TreeHeader::ToBytes() const
{
  auto bytes = DynamicArray<byte>(this->page_size);
  write_le_uint16<byte>(bytes, TREE_HEADER_MAGIC_OFFSET, static_cast<uint16_t>(this->tree_type));
  write_le_uint16<byte>(bytes, MAX_ENTRIES_OFFSET, this->max_node_entries);
  write_le_uint16<byte>(bytes, MAX_RECORDS_OFFSET, this->max_node_records);
  write_le_uint32<byte>(bytes, ROOT_NODE_PAGE_OFFSET, this->root);
  write_le_uint32<byte>(bytes, PAGE_COUNT_OFFSET, this->page_count);

  return bytes;
}

/*** TreeHeaderBuilder ***/

TreeHeaderBuilder::TreeHeaderBuilder()
    :page_size(NoidConfig::Get().vfs_page_size), tree_type(TreeType::None),
     max_node_entries(CalculateMaxEntries(this->page_size)), max_node_records(CalculateMaxRecords(this->page_size)),
     root(0), page_count(0) { }

TreeHeaderBuilder::TreeHeaderBuilder(const TreeHeader& base)
    :page_size(NoidConfig::Get().vfs_page_size), tree_type(base.tree_type), max_node_entries(base.max_node_entries),
     max_node_records(base.max_node_records), root(base.root), page_count(base.page_count) { }

TreeHeaderBuilder::TreeHeaderBuilder(DynamicArray<byte>&& base)
    :page_size(NoidConfig::Get().vfs_page_size),
     tree_type(static_cast<TreeType>(read_le_uint16<byte>(base, TREE_HEADER_MAGIC_OFFSET))),
     max_node_entries(read_le_uint16<byte>(base, MAX_ENTRIES_OFFSET)),
     max_node_records(read_le_uint16<byte>(base, MAX_RECORDS_OFFSET)),
     root(read_le_uint32<byte>(base, ROOT_NODE_PAGE_OFFSET)),
     page_count(read_le_uint32<byte>(base, PAGE_COUNT_OFFSET)) { }

std::unique_ptr<const TreeHeader> TreeHeaderBuilder::Build()
{
  if (this->tree_type == TreeType::None) {
    throw std::domain_error("Tree type of TreeHeader not set.");
  }

  return std::unique_ptr<const TreeHeader>(
      new TreeHeader(this->tree_type, this->max_node_entries, this->max_node_records, this->root, this->page_count,
          this->page_size));
}

TreeHeaderBuilder& TreeHeaderBuilder::WithTreeType(TreeType type)
{
  if (this->tree_type == TreeType::None || this->tree_type == type) {
    this->tree_type = type;
    return *this;
  }

  throw std::domain_error("Update of tree type not allowed once it has been set.");
}

TreeHeaderBuilder& TreeHeaderBuilder::WithRootPageNumber(PageNumber root_page)
{
  if (this->root == 0 || this->root == root_page) {
    this->root = root_page;
    return *this;
  }

  throw std::domain_error("Update of root page not allowed once it has been set.");
}

TreeHeaderBuilder& TreeHeaderBuilder::WithPageCount(uint32_t count)
{
  this->page_count = count;

  return *this;
}

}
/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <bit>
#include <cmath>
#include <stdexcept>

#include "FileHeader.h"
#include "backend/Bits.h"

namespace noid::backend::page {

static const uint64_t NOID_DATABASE_HEADER_MAGIC = 0x0031762064696f6e;
static const uint8_t MAGIC_OFFSET = 0;
static const uint8_t PAGE_SIZE_OFFSET = 8;
static const uint8_t KEY_SIZE_OFFSET = 10;
static const uint8_t FIRST_TREE_HEADER_PAGE_NUMBER_OFFSET = 11;
static const uint8_t FIRST_FREELIST_PAGE_NUMBER_OFFSET = 15;
static const uint8_t CHECKSUM_OFFSET = 19;

static std::array<byte, FileHeader::SIZE> const& Validate(
    std::array<byte, FileHeader::SIZE> const& data)
{
  auto expected_checksum = fnv1a<byte>(data, 0, CHECKSUM_OFFSET);
  auto actual_checksum = read_le_uint32<byte>(data, CHECKSUM_OFFSET);

  if (actual_checksum != expected_checksum) {
    throw std::invalid_argument("invalid checksum");
  }

  return data;
}

FileHeader::FileHeader(uint16_t page_size, uint8_t key_size, PageNumber first_tree_header_page,
    PageNumber first_freelist_page, uint32_t checksum)
    :
    page_size(page_size), key_size(key_size), first_tree_header_page(first_tree_header_page),
    first_freelist_page(first_freelist_page), checksum(checksum) { }

std::shared_ptr<FileHeaderBuilder> FileHeader::NewBuilder()
{
  return std::shared_ptr<FileHeaderBuilder>(new FileHeaderBuilder());
}

std::shared_ptr<FileHeaderBuilder> FileHeader::NewBuilder(const FileHeader& base)
{
  return std::shared_ptr<FileHeaderBuilder>(new FileHeaderBuilder(base));
}

std::shared_ptr<FileHeaderBuilder> FileHeader::NewBuilder(std::array<byte, FileHeader::SIZE>& base)
{
  return std::shared_ptr<FileHeaderBuilder>(new FileHeaderBuilder(Validate(base)));
}

std::array<byte, FileHeader::SIZE> FileHeader::ToBytes() const
{
  std::array<byte, FileHeader::SIZE> bytes = {0};
  write_le_uint64<byte>(bytes, MAGIC_OFFSET, NOID_DATABASE_HEADER_MAGIC);
  write_le_uint16<byte>(bytes, PAGE_SIZE_OFFSET, this->page_size);
  write_uint8<byte>(bytes, KEY_SIZE_OFFSET, this->key_size);
  write_le_uint32<byte>(bytes, FIRST_TREE_HEADER_PAGE_NUMBER_OFFSET, this->first_tree_header_page);
  write_le_uint32<byte>(bytes, FIRST_FREELIST_PAGE_NUMBER_OFFSET, this->first_freelist_page);
  write_le_uint32<byte>(bytes, CHECKSUM_OFFSET, this->checksum);

  return bytes;
}

uint16_t FileHeader::GetPageSize() const
{
  return this->page_size;
}

uint8_t FileHeader::GetKeySize() const
{
  return this->key_size;
}

PageNumber FileHeader::GetFirstTreeHeaderPage() const
{
  return this->first_tree_header_page;
}

PageNumber FileHeader::GetFirstFreelistPage() const
{
  return this->first_freelist_page;
}

uint32_t FileHeader::GetChecksum() const
{
  return this->checksum;
}

bool FileHeader::Equals(const FileHeader& other) const
{
  return this->checksum == other.checksum && this->page_size == other.page_size && this->key_size == other.key_size
      && this->first_tree_header_page == other.first_tree_header_page
      && this->first_freelist_page == other.first_freelist_page;
}

bool operator==(const FileHeader& lhs, const FileHeader& rhs)
{
  return lhs.Equals(rhs);
}

bool operator!=(const FileHeader& lhs, const FileHeader& rhs)
{
  return !lhs.Equals(rhs);
}

/*** FileHeaderBuilder ***/

FileHeaderBuilder::FileHeaderBuilder(const noid::backend::page::FileHeader& base)
    :page_size(base.page_size), key_size(base.key_size), first_tree_header_page(base.first_tree_header_page),
     first_freelist_page(base.first_freelist_page) { }

FileHeaderBuilder::FileHeaderBuilder(std::array<byte, FileHeader::SIZE> const& base)
    :page_size(read_le_uint16<byte>(base, PAGE_SIZE_OFFSET)), key_size(read_uint8<byte>(base, KEY_SIZE_OFFSET)),
     first_tree_header_page(read_le_uint32<byte>(base, FIRST_TREE_HEADER_PAGE_NUMBER_OFFSET)),
     first_freelist_page(read_le_uint32<byte>(base, FIRST_FREELIST_PAGE_NUMBER_OFFSET)) { }

std::unique_ptr<const FileHeader> FileHeaderBuilder::Build() const
{
  auto checksum = FNV1a::Init()
      .Iterate(NOID_DATABASE_HEADER_MAGIC)
      .Iterate(this->page_size)
      .Iterate(this->key_size)
      .Iterate(this->first_tree_header_page)
      .Iterate(this->first_freelist_page)
      .GetState();

  return std::unique_ptr<FileHeader>(new FileHeader(
      this->page_size, this->key_size, this->first_tree_header_page, this->first_freelist_page, checksum));
}

std::shared_ptr<FileHeaderBuilder> FileHeaderBuilder::WithPageSize(uint16_t size)
{
  auto p2 = safe_round_to_next_power_of_2(size);
  this->page_size = p2 < 512 ? 512 : p2;

  return this->shared_from_this();
}

std::shared_ptr<FileHeaderBuilder> FileHeaderBuilder::WithKeySize(uint8_t size)
{
  this->key_size = safe_next_multiple_of_8(size);

  return this->shared_from_this();
}

std::shared_ptr<FileHeaderBuilder> FileHeaderBuilder::WithFirstTreeHeaderPage(PageNumber page_number)
{
  this->first_tree_header_page = page_number;

  return this->shared_from_this();
}

std::shared_ptr<FileHeaderBuilder> FileHeaderBuilder::WithFirstFreeListPage(PageNumber page_number)
{
  this->first_freelist_page = page_number;

  return this->shared_from_this();
}

}
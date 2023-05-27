/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <cmath>
#include <stdexcept>

#include "DatabaseHeader.h"
#include "backend/Bits.h"

#define NOID_DATABASE_HEADER_MAGIC {'n', 'o', 'i', 'd', ' ', 'v', '1', '\0'}

static uint8_t PAGE_SIZE_OFFSET = 8;
static uint8_t KEY_SIZE_OFFSET = 10;
static uint8_t FIRST_TREE_HEADER_PAGE_NUMBER_OFFSET = 11;
static uint8_t FIRST_FREELIST_PAGE_NUMBER_OFFSET = 15;
static uint8_t CHECKSUM_OFFSET = 19;

namespace noid::backend::page {

static std::array<byte, DatabaseHeader::BYTE_SIZE> const & validate(std::array<byte, DatabaseHeader::BYTE_SIZE> const & data) {
  auto expected_checksum = fnv1a<DatabaseHeader::BYTE_SIZE>(data, 0, CHECKSUM_OFFSET);
  auto actual_checksum = read_le_uint32(data, CHECKSUM_OFFSET);

  if (actual_checksum != expected_checksum) {
    throw std::invalid_argument("invalid checksum");
  }

  return data;
}

DatabaseHeader::DatabaseHeader(std::array<byte, DatabaseHeader::BYTE_SIZE> const data) : data(data) {}

std::unique_ptr<DatabaseHeaderBuilder> DatabaseHeader::NewBuilder() {
  return DatabaseHeaderBuilder::Create();
}

std::unique_ptr<DatabaseHeaderBuilder> DatabaseHeader::NewBuilder(std::array<byte, DatabaseHeader::BYTE_SIZE> &base) {
  return DatabaseHeaderBuilder::Create(base);
}

std::unique_ptr<DatabaseHeaderBuilder> DatabaseHeader::NewBuilder(const DatabaseHeader& base) {
  return DatabaseHeaderBuilder::Create(base);
}

const std::array<byte, DatabaseHeader::BYTE_SIZE> &DatabaseHeader::GetBytes() const {
  return this->data;
}

uint16_t DatabaseHeader::GetPageSize() const {
  return read_le_uint16(this->data, PAGE_SIZE_OFFSET);
}

uint8_t DatabaseHeader::GetKeySize() const {
  return read_uint8(this->data, KEY_SIZE_OFFSET);
}

PageNumber DatabaseHeader::GetFirstTreeHeaderPage() const {
  return read_le_uint32(this->data, FIRST_TREE_HEADER_PAGE_NUMBER_OFFSET);
}

PageNumber DatabaseHeader::GetFirstFreelistPage() const {
  return read_le_uint32(this->data, FIRST_FREELIST_PAGE_NUMBER_OFFSET);
}

uint32_t DatabaseHeader::GetSignature() const {
  return read_le_uint32(this->data, CHECKSUM_OFFSET);
}

bool DatabaseHeader::Equals(const DatabaseHeader &other) const {
  return this->data == other.data;
}

bool operator==(const DatabaseHeader& lhs, const DatabaseHeader& rhs) {
  return lhs.Equals(rhs);
}

bool operator!=(const DatabaseHeader& lhs, const DatabaseHeader& rhs) {
  return !lhs.Equals(rhs);
}

/*** DatabaseHeaderBuilder ***/

DatabaseHeaderBuilder::DatabaseHeaderBuilder() :
  page_size(DEFAULT_PAGE_SIZE), key_size(DEFAULT_KEY_SIZE), first_tree_header_page(0), first_freelist_page(0) {}

DatabaseHeaderBuilder::DatabaseHeaderBuilder(std::array<byte, DatabaseHeader::BYTE_SIZE> const & base) {
  this->page_size = read_le_uint16(base, PAGE_SIZE_OFFSET);
  this->key_size = read_uint8(base, KEY_SIZE_OFFSET);
  this->first_tree_header_page = read_le_uint32(base, FIRST_TREE_HEADER_PAGE_NUMBER_OFFSET);
  this->first_freelist_page = read_le_uint32(base, FIRST_FREELIST_PAGE_NUMBER_OFFSET);
}

std::unique_ptr<DatabaseHeaderBuilder> DatabaseHeaderBuilder::Create() {
  return std::unique_ptr<DatabaseHeaderBuilder>(new DatabaseHeaderBuilder());
}

std::unique_ptr<DatabaseHeaderBuilder> DatabaseHeaderBuilder::Create(std::array<byte, DatabaseHeader::BYTE_SIZE> const & base) {
  return std::unique_ptr<DatabaseHeaderBuilder>(new DatabaseHeaderBuilder(validate(base)));
}

std::unique_ptr<DatabaseHeaderBuilder> DatabaseHeaderBuilder::Create(const DatabaseHeader& base) {
  return std::unique_ptr<DatabaseHeaderBuilder>(new DatabaseHeaderBuilder(base.data));
}

std::unique_ptr<const DatabaseHeader> DatabaseHeaderBuilder::Build() const {
  std::array<byte, DatabaseHeader::BYTE_SIZE> data = NOID_DATABASE_HEADER_MAGIC;
  write_le_uint16(data, PAGE_SIZE_OFFSET, this->page_size);
  write_uint8(data, KEY_SIZE_OFFSET, this->key_size);
  write_le_uint32(data, FIRST_TREE_HEADER_PAGE_NUMBER_OFFSET, this->first_tree_header_page);
  write_le_uint32(data, FIRST_FREELIST_PAGE_NUMBER_OFFSET, this->first_freelist_page);
  write_le_uint32(data, CHECKSUM_OFFSET, fnv1a<DatabaseHeader::BYTE_SIZE>(data, 0, CHECKSUM_OFFSET));

  return std::unique_ptr<DatabaseHeader>(new DatabaseHeader(data));
}

DatabaseHeaderBuilder &DatabaseHeaderBuilder::WithPageSize(uint16_t size) {
  auto p2 = safe_round_to_next_power_of_2(size);
  this->page_size = p2 < 512 ? 512 : p2;

  return *this;
}

DatabaseHeaderBuilder &DatabaseHeaderBuilder::WithKeySize(uint8_t size) {
  this->key_size = safe_next_multiple_of_8(size);

  return *this;
}

DatabaseHeaderBuilder &DatabaseHeaderBuilder::WithFirstTreeHeaderPage(PageNumber page_number) {
  this->first_tree_header_page = page_number;

  return *this;
}

DatabaseHeaderBuilder &DatabaseHeaderBuilder::WithFirstFreeListPage(PageNumber page_number) {
  this->first_freelist_page = page_number;

  return *this;
}

}
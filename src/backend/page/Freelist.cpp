/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "Freelist.h"
#include "backend/Bits.h"
#include "backend/NoidConfig.h"

static uint8_t PREVIOUS_PAGE_OFFSET = 2;
static uint8_t NEXT_PAGE_OFFSET = 6;
static uint8_t NEXT_FREE_SLOT_OFFSET = 10;
static uint8_t FREELIST_OFFSET = 12;

namespace noid::backend::page {

static std::vector<byte>& Validate(std::vector<byte>& data) {
  if (data.size() == NoidConfig::Get().vfs_page_size && data[0] == 'F' && data[1] == 'L') {
    return data;
  }

  throw std::invalid_argument("Given raw data is not a valid serialized Freelist instance.");
}

static inline std::size_t PositionToIndex(uint16_t position) {
  return FREELIST_OFFSET + position * sizeof(PageNumber);
}

Freelist::Freelist(std::vector<byte> data) : data(std::move(data)) {}

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder() {
  return FreelistBuilder::Create();
}

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder(const Freelist &base) {
  return FreelistBuilder::Create(base);
}

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder(std::vector<byte> && base) {
  return FreelistBuilder::Create(std::move(base));
}

PageNumber Freelist::Previous() const {
  return read_le_uint32<byte>(this->data, PREVIOUS_PAGE_OFFSET);
}

PageNumber Freelist::Next() const {
  return read_le_uint32<byte>(this->data, NEXT_PAGE_OFFSET);
}

uint16_t Freelist::Size() const {
  return read_le_uint16<byte>(this->data, NEXT_FREE_SLOT_OFFSET);
}

PageNumber Freelist::FreePageAt(uint16_t pos) const {
  if (pos >= this->Size()) {
    throw std::out_of_range("No such page");
  }

  return read_le_uint32<byte>(this->data, PositionToIndex(pos));
}

FreelistBuilder::FreelistBuilder() :
    page_size(NoidConfig::Get().vfs_page_size),
    max_page_position(((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1),
    next_free_page_position(0),
    data(this->page_size) {
  data[0] = 'F';
  data[1] = 'L';
}

FreelistBuilder::FreelistBuilder(const Freelist &base) :
    page_size(base.data.size()),
    max_page_position(((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1),
    next_free_page_position(read_le_uint16<byte>(base.data, NEXT_FREE_SLOT_OFFSET)),
    data(base.data) {}

FreelistBuilder::FreelistBuilder(std::vector<byte> && base) {
  this->data = std::move(base);
  this->page_size = this->data.size();
  this->max_page_position = ((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1;
  this->next_free_page_position = read_le_uint16<byte>(this->data, NEXT_FREE_SLOT_OFFSET);
}

std::unique_ptr<FreelistBuilder> FreelistBuilder::Create() {
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder());
}

std::unique_ptr<FreelistBuilder> FreelistBuilder::Create(const Freelist &base) {
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder(base));
}

std::unique_ptr<FreelistBuilder> FreelistBuilder::Create(std::vector<byte> && base) {
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder(std::move(Validate(base))));
}

std::unique_ptr<const Freelist> FreelistBuilder::Build() const {
  return std::unique_ptr<const Freelist>(new Freelist(this->data));
}

FreelistBuilder &FreelistBuilder::WithPrevious(PageNumber previous) {
  write_le_uint32<byte>(this->data, PREVIOUS_PAGE_OFFSET, previous);

  return *this;
}

FreelistBuilder &FreelistBuilder::WithNext(PageNumber next) {
  write_le_uint32<byte>(this->data, NEXT_PAGE_OFFSET, next);

  return *this;
}

FreelistBuilder &FreelistBuilder::WithFreePage(PageNumber free_page) {
  if (this->next_free_page_position > this->max_page_position) {
    throw std::overflow_error("Freelist overflow.");
  }

  write_le_uint32<byte>(this->data, PositionToIndex(this->next_free_page_position), free_page);

  this->next_free_page_position++;
  write_le_uint16<byte>(this->data, NEXT_FREE_SLOT_OFFSET, this->next_free_page_position);

  return *this;
}

FreelistBuilder &FreelistBuilder::WithFreePage(PageNumber free_page, std::size_t position_hint) {
  if (position_hint < this->next_free_page_position) {
    write_le_uint32<byte>(this->data, PositionToIndex(position_hint), free_page);
    return *this;
  }

  return this->WithFreePage(free_page);
}

}
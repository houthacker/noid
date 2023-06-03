/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "Freelist.h"
#include "backend/Bits.h"
#include "backend/NoidConfig.h"

static uint16_t FREELIST_MAGIC = 0x4c46; // 'FL' for Freelist
static uint8_t PREVIOUS_PAGE_OFFSET = 2;
static uint8_t NEXT_PAGE_OFFSET = 6;
static uint8_t NEXT_FREE_SLOT_OFFSET = 10;
static uint8_t FREELIST_OFFSET = 12;

namespace noid::backend::page {

static std::vector<byte>& Validate(std::vector<byte>& data) {
  if (data.size() == NoidConfig::Get().vfs_page_size && read_le_uint16<byte>(data, 0) == FREELIST_MAGIC) {
    return data;
  }

  throw std::invalid_argument("Given raw data is not a valid serialized Freelist instance.");
}

static inline std::size_t SlotToIndex(uint16_t slot) {
  return FREELIST_OFFSET + slot * sizeof(PageNumber);
}

Freelist::Freelist(PageNumber previous, PageNumber next, std::vector<PageNumber> free_pages) :
  previous(previous), next(next), free_pages(std::move(free_pages)) {}

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder() {
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder());
}

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder(const Freelist &base) {
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder(base));
}

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder(std::vector<byte> && base) {
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder(std::move(Validate(base))));
}

PageNumber Freelist::Previous() const {
  return this->previous;
}

PageNumber Freelist::Next() const {
  return this->next;
}

uint16_t Freelist::Size() const {
  return this->free_pages.size();
}

PageNumber Freelist::FreePageAt(uint16_t pos) const {
  return this->free_pages.at(pos);
}

FreelistBuilder::FreelistBuilder() :
    previous_freelist_entry(0),
    next_freelist_entry(0),
    page_size(NoidConfig::Get().vfs_page_size),
    max_slot(((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1) {}

FreelistBuilder::FreelistBuilder(const Freelist &base) :
    previous_freelist_entry(base.previous),
    next_freelist_entry(base.next),
    page_size(NoidConfig::Get().vfs_page_size),
    max_slot(((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1),
    free_pages(base.free_pages) {}

FreelistBuilder::FreelistBuilder(std::vector<byte> && base) :
    previous_freelist_entry(read_le_uint32<byte>(base, PREVIOUS_PAGE_OFFSET)),
    next_freelist_entry(read_le_uint32<byte>(base, NEXT_PAGE_OFFSET)),
    page_size(NoidConfig::Get().vfs_page_size),
    max_slot(((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1) {

  auto next_free_slot = read_le_uint16<byte>(base, NEXT_FREE_SLOT_OFFSET);
  for (int slot = 0; slot < next_free_slot; slot++) {
    this->free_pages.push_back(read_le_uint32<byte>(base, SlotToIndex(slot)));
  }
}

std::unique_ptr<const Freelist> FreelistBuilder::Build() const {
  return std::unique_ptr<const Freelist>(new Freelist(
      this->previous_freelist_entry,
      this->next_freelist_entry,
      this->free_pages));
}

FreelistBuilder &FreelistBuilder::WithPrevious(PageNumber previous) {
  this->previous_freelist_entry = previous;

  return *this;
}

FreelistBuilder &FreelistBuilder::WithNext(PageNumber next) {
  this->next_freelist_entry = next;

  return *this;
}

FreelistBuilder &FreelistBuilder::WithFreePage(PageNumber free_page) {
  if (this->free_pages.size() - 1 == this->max_slot) {
    throw std::overflow_error("Freelist overflow.");
  }

  this->free_pages.push_back(free_page);
  return *this;
}

FreelistBuilder &FreelistBuilder::WithFreePage(PageNumber free_page, std::size_t slot_hint) {
  if (slot_hint < this->free_pages.size()) {
    this->free_pages[slot_hint] = free_page;
    return *this;
  }

  return this->WithFreePage(free_page);
}

}
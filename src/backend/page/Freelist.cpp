/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "Freelist.h"
#include "backend/Bits.h"

static const uint16_t FREELIST_MAGIC = 0x4c46; // 'FL' for Freelist
static const uint8_t MAGIC_OFFSET = 0;
static const uint8_t PREVIOUS_PAGE_OFFSET = 2;
static const uint8_t NEXT_PAGE_OFFSET = 6;
static const uint8_t NEXT_FREE_SLOT_OFFSET = 10;

namespace noid::backend::page {

static DynamicArray<byte>& Validate(DynamicArray<byte>& data, uint16_t page_size)
{
  if (data.size() != page_size) {
    std::stringstream stream;
    stream << "Invalid data size (expect " << +page_size << ", but got " << +data.size() << ").";

    throw std::invalid_argument(stream.str());
  }
  else if (read_le_uint16<byte>(data, 0) != FREELIST_MAGIC) {
    throw std::invalid_argument("Invalid Freelist magic");
  }

  return data;
}

static inline std::size_t SlotToIndex(uint16_t slot)
{
  return Freelist::HEADER_SIZE + slot * sizeof(PageNumber);
}

Freelist::Freelist(PageNumber location, uint16_t page_size, PageNumber previous, PageNumber next,
    DynamicArray<PageNumber> free_pages, uint16_t next_free_slot)
    :location(location), page_size(page_size), previous(previous), next(next), free_pages(std::move(free_pages)),
     next_free_slot(next_free_slot) { }

std::shared_ptr<FreelistBuilder> Freelist::NewBuilder(uint16_t page_size)
{
  return std::shared_ptr<FreelistBuilder>(new FreelistBuilder(page_size));
}

std::shared_ptr<FreelistBuilder> Freelist::NewBuilder(const Freelist& base)
{
  return std::shared_ptr<FreelistBuilder>(new FreelistBuilder(base));
}

std::shared_ptr<FreelistBuilder> Freelist::NewBuilder(DynamicArray<byte>&& base)
{
  return std::shared_ptr<FreelistBuilder>(new FreelistBuilder(std::move(Validate(base, base.size()))));
}

PageNumber Freelist::GetLocation() const
{
  return this->location;
}

PageNumber Freelist::Previous() const
{
  return this->previous;
}

PageNumber Freelist::Next() const
{
  return this->next;
}

uint16_t Freelist::Size() const
{
  return this->next_free_slot;
}

PageNumber Freelist::FreePageAt(uint16_t pos) const
{
  if (pos >= this->next_free_slot) {
    throw std::out_of_range("pos");
  }

  return this->free_pages[pos];
}

DynamicArray<byte> Freelist::ToBytes() const
{
  auto serialized = DynamicArray<byte>(this->page_size);
  write_le_uint32<byte>(serialized, MAGIC_OFFSET, FREELIST_MAGIC);
  write_le_uint32<byte>(serialized, PREVIOUS_PAGE_OFFSET, this->previous);
  write_le_uint32<byte>(serialized, NEXT_PAGE_OFFSET, this->next);
  write_le_uint16<byte>(serialized, NEXT_FREE_SLOT_OFFSET, this->next_free_slot);
  write_contiguous_container<PageNumber>(serialized, Freelist::HEADER_SIZE, this->free_pages);

  return serialized;
}

FreelistBuilder::FreelistBuilder(uint16_t size)
    :previous_freelist_entry(0), next_freelist_entry(0), page_size(size),
     max_slot(((page_size - Freelist::HEADER_SIZE) / sizeof(PageNumber)) - 1), cursor(0), free_pages(0) { }

FreelistBuilder::FreelistBuilder(const Freelist& base)
    :location(base.location), previous_freelist_entry(base.previous), next_freelist_entry(base.next), page_size(base.page_size),
     max_slot(((page_size - Freelist::HEADER_SIZE) / sizeof(PageNumber)) - 1), cursor(base.next_free_slot),
     free_pages(base.next_free_slot)
{
  std::ranges::copy(base.free_pages.begin(), base.free_pages.begin() + base.next_free_slot, this->free_pages.begin());
}

FreelistBuilder::FreelistBuilder(DynamicArray<byte>&& base)
    :previous_freelist_entry(read_le_uint32<byte>(base, PREVIOUS_PAGE_OFFSET)),
     next_freelist_entry(read_le_uint32<byte>(base, NEXT_PAGE_OFFSET)), page_size(safe_cast<uint16_t>(base.size())),
     max_slot(((page_size - Freelist::HEADER_SIZE) / sizeof(PageNumber)) - 1), cursor(0),
     free_pages(read_le_uint16<byte>(base, NEXT_FREE_SLOT_OFFSET))
{

  for (; this->cursor < this->free_pages.size(); this->cursor++) {
    this->free_pages[cursor] = read_le_uint32<byte>(base, SlotToIndex(cursor));
  }
}

std::unique_ptr<const Freelist> FreelistBuilder::Build() const
{
  if (this->location == NULL_PAGE) {
    throw std::domain_error("Cannot build Freelist: location is unset.");
  }

  auto free_list = DynamicArray<PageNumber>((this->page_size - Freelist::HEADER_SIZE) / sizeof(PageNumber));
  std::ranges::copy(this->free_pages.begin(), this->free_pages.end(), free_list.begin());

  return std::unique_ptr<const Freelist>(
      new Freelist(this->location, this->page_size, this->previous_freelist_entry, this->next_freelist_entry,
          std::move(free_list), this->free_pages.size()));
}

std::shared_ptr<FreelistBuilder> FreelistBuilder::WithLocation(PageNumber loc)
{
  if (loc == NULL_PAGE) {
    throw std::domain_error("GetLocation cannot be zero.");
  }

  this->location = loc;
  return this->shared_from_this();
}

std::shared_ptr<FreelistBuilder> FreelistBuilder::WithPrevious(PageNumber previous)
{
  this->previous_freelist_entry = previous;

  return this->shared_from_this();
}

std::shared_ptr<FreelistBuilder> FreelistBuilder::WithNext(PageNumber next)
{
  this->next_freelist_entry = next;

  return this->shared_from_this();
}

std::shared_ptr<FreelistBuilder> FreelistBuilder::WithFreePage(PageNumber free_page)
{
  if (this->cursor - 1 == this->max_slot) {
    throw std::overflow_error("Freelist overflow.");
  }

  this->free_pages.push_back(free_page);
  this->cursor++;
  return this->shared_from_this();
}

std::shared_ptr<FreelistBuilder> FreelistBuilder::WithFreePage(PageNumber free_page, std::size_t slot_hint)
{
  if (slot_hint <= this->cursor) {
    this->free_pages[slot_hint] = free_page;
    return this->shared_from_this();
  }

  return this->WithFreePage(free_page);
}

}
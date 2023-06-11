/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <algorithm>
#include <sstream>
#include <string>

#include "Freelist.h"
#include "backend/Bits.h"
#include "backend/NoidConfig.h"

static uint16_t FREELIST_MAGIC = 0x4c46; // 'FL' for Freelist
static uint8_t MAGIC_OFFSET = 0;
static uint8_t PREVIOUS_PAGE_OFFSET = 2;
static uint8_t NEXT_PAGE_OFFSET = 6;
static uint8_t NEXT_FREE_SLOT_OFFSET = 10;
static uint8_t FREELIST_OFFSET = 12;

namespace noid::backend::page {

static DynamicArray<byte>& Validate(DynamicArray<byte>& data)
{
  if (data.size() != NoidConfig::Get().vfs_page_size) {
    std::stringstream stream;
    stream << "Invalid data size (expect " << NoidConfig::Get().vfs_page_size << ", but got " << +data.size() << ").";

    throw std::invalid_argument(stream.str());
  }
  else if (read_le_uint16<byte>(data, 0) != FREELIST_MAGIC) {
    throw std::invalid_argument("Invalid Freelist magic");
  }

  return data;
}

static inline std::size_t SlotToIndex(uint16_t slot)
{
  return FREELIST_OFFSET + slot * sizeof(PageNumber);
}

Freelist::Freelist(uint16_t page_size, PageNumber previous, PageNumber next, DynamicArray<PageNumber> free_pages)
    :page_size(page_size), previous(previous), next(next), free_pages(std::move(free_pages)) { }

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder()
{
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder());
}

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder(const Freelist& base)
{
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder(base));
}

std::unique_ptr<FreelistBuilder> Freelist::NewBuilder(DynamicArray<byte>&& base)
{
  return std::unique_ptr<FreelistBuilder>(new FreelistBuilder(std::move(Validate(base))));
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
  return this->free_pages.size();
}

PageNumber Freelist::FreePageAt(uint16_t pos) const
{
  return this->free_pages.at(pos);
}

DynamicArray<byte> Freelist::ToBytes() const
{
  auto serialized = DynamicArray<byte>(this->page_size);
  write_le_uint32<byte>(serialized, MAGIC_OFFSET, FREELIST_MAGIC);
  write_le_uint32<byte>(serialized, PREVIOUS_PAGE_OFFSET, this->previous);
  write_le_uint32<byte>(serialized, NEXT_PAGE_OFFSET, this->next);
  write_le_uint16<byte>(serialized, NEXT_FREE_SLOT_OFFSET, static_cast<uint16_t>(this->free_pages.size()));
  write_contiguous_container<PageNumber>(serialized, FREELIST_OFFSET, this->free_pages);

  return serialized;
}

FreelistBuilder::FreelistBuilder()
    :
    previous_freelist_entry(0),
    next_freelist_entry(0),
    page_size(NoidConfig::Get().vfs_page_size),
    max_slot(((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1),
    cursor(0),
    free_pages(0) { }

FreelistBuilder::FreelistBuilder(const Freelist& base)
    :
    previous_freelist_entry(base.previous),
    next_freelist_entry(base.next),
    page_size(NoidConfig::Get().vfs_page_size),
    max_slot(((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1),
    cursor(base.Size()),
    free_pages(base.free_pages.size())
{
  std::copy(base.free_pages.begin(), base.free_pages.end(), this->free_pages.begin());
}

FreelistBuilder::FreelistBuilder(DynamicArray<byte>&& base)
    :
    previous_freelist_entry(read_le_uint32<byte>(base, PREVIOUS_PAGE_OFFSET)),
    next_freelist_entry(read_le_uint32<byte>(base, NEXT_PAGE_OFFSET)),
    page_size(NoidConfig::Get().vfs_page_size),
    max_slot(((page_size - FREELIST_OFFSET) / sizeof(PageNumber)) - 1),
    cursor(0),
    free_pages(read_le_uint16<byte>(base, NEXT_FREE_SLOT_OFFSET))
{

  for (; this->cursor < this->free_pages.size(); this->cursor++) {
    this->free_pages[cursor] = read_le_uint32<byte>(base, SlotToIndex(cursor));
  }
}

std::unique_ptr<const Freelist> FreelistBuilder::Build() const
{
  auto free_list = DynamicArray<PageNumber>(this->free_pages.size());
  std::copy(this->free_pages.begin(), this->free_pages.end(), free_list.begin());

  return std::unique_ptr<const Freelist>(new Freelist(
      this->page_size,
      this->previous_freelist_entry,
      this->next_freelist_entry,
      std::move(free_list)));
}

FreelistBuilder& FreelistBuilder::WithPrevious(PageNumber previous)
{
  this->previous_freelist_entry = previous;

  return *this;
}

FreelistBuilder& FreelistBuilder::WithNext(PageNumber next)
{
  this->next_freelist_entry = next;

  return *this;
}

FreelistBuilder& FreelistBuilder::WithFreePage(PageNumber free_page)
{
  if (this->cursor - 1 == this->max_slot) {
    throw std::overflow_error("Freelist overflow.");
  }

  this->free_pages.push_back(free_page);
  this->cursor++;
  return *this;
}

FreelistBuilder& FreelistBuilder::WithFreePage(PageNumber free_page, std::size_t slot_hint)
{
  if (slot_hint <= this->cursor) {
    this->free_pages[slot_hint] = free_page;
    return *this;
  }

  return this->WithFreePage(free_page);
}

}
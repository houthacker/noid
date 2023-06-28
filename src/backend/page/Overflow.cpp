/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "Overflow.h"
#include "backend/Bits.h"

namespace noid::backend::page {

static const uint8_t DATA_SIZE_OFFSET = 0;
static const uint8_t NEXT_OVERFLOW_PAGE_OFFSET = 2;
static const uint8_t DATA_OFFSET = Overflow::HEADER_SIZE;

static DynamicArray<byte>& Validate(DynamicArray<byte>& data, uint16_t page_size)
{
  if (data.size() < page_size) {
    throw std::invalid_argument("Given raw data has an invalid page size.");
  }

  return data;
}

Overflow::Overflow(PageNumber location, uint16_t payload_size, PageNumber next, DynamicArray<byte>&& data,
    uint16_t page_size)
    :location(location), data_size(payload_size), next(next), data(std::move(data)), page_size(page_size) { }

std::shared_ptr<OverflowBuilder> Overflow::NewBuilder(uint16_t page_size)
{
  if (page_size <= Overflow::HEADER_SIZE) {
    throw std::length_error("Page size too small to contain overflow data.");
  }

  return std::shared_ptr<OverflowBuilder>(new OverflowBuilder(page_size));
}

std::shared_ptr<OverflowBuilder> Overflow::NewBuilder(const Overflow& base)
{
  return std::shared_ptr<OverflowBuilder>(new OverflowBuilder(base));
}

std::shared_ptr<OverflowBuilder> Overflow::NewBuilder(DynamicArray<byte>&& base)
{
  return std::shared_ptr<OverflowBuilder>(
      new OverflowBuilder(std::move(Validate(base, safe_cast<uint16_t>(base.size())))));
}

PageNumber Overflow::GetLocation() const
{
  return this->location;
}

uint16_t Overflow::GetDataSize() const
{
  return this->data_size;
}

PageNumber Overflow::GetNext() const
{
  return this->next;
}

const DynamicArray<byte>& Overflow::GetData() const
{
  return this->data;
}

DynamicArray<byte> Overflow::ToBytes() const
{
  auto bytes = DynamicArray<byte>(this->page_size);

  write_le_uint16<byte>(bytes, DATA_SIZE_OFFSET, this->data_size);
  write_le_uint32<byte>(bytes, NEXT_OVERFLOW_PAGE_OFFSET, this->next);
  write_contiguous_container<byte>(bytes, DATA_OFFSET, this->data);

  return bytes;
}

/*** OverflowBuilder ***/


OverflowBuilder::OverflowBuilder(uint16_t page_size)
    :page_size(page_size) { }

OverflowBuilder::OverflowBuilder(const Overflow& base)
    :location(base.location), page_size(base.page_size), data_size(base.data_size), next(base.next), data(base.data) { }

OverflowBuilder::OverflowBuilder(DynamicArray<byte>&& base)
    :page_size(static_cast<decltype(page_size)>(base.size())),
     data_size(read_le_uint16<byte>(base, DATA_SIZE_OFFSET)),
     next(read_le_uint32<byte>(base, NEXT_OVERFLOW_PAGE_OFFSET)),
     data(page_size - DATA_OFFSET)
{
  write_contiguous_container<byte>(this->data, 0, base, DATA_OFFSET);
}

uint16_t OverflowBuilder::MaxDataSize() const
{
  if (this->page_size < Overflow::HEADER_SIZE) {
    return 0;
  }

  return this->page_size - Overflow::HEADER_SIZE;
}

std::unique_ptr<const Overflow> OverflowBuilder::Build()
{
  if (this->location == NULL_PAGE) {
    throw std::domain_error("Cannot build Overflow: location not set.");
  }
  else if (this->page_size <= Overflow::HEADER_SIZE) {
    throw std::length_error("Cannot build Overflow: page_size too small.");
  }
  else if (this->data_size == 0) {
    throw std::length_error("Cannot build Overflow: data_size is zero.");
  }

  DynamicArray<byte> padded_data = {0};
  if (this->data.size() < this->MaxDataSize()) {
    padded_data = DynamicArray<byte>(this->MaxDataSize());
    write_container<byte>(padded_data, 0, this->data);
  }
  else {
    padded_data = std::move(this->data);
  }

  return std::unique_ptr<const Overflow>(
      new Overflow(this->location, this->data_size, this->next, std::move(padded_data), this->page_size));
}

std::shared_ptr<OverflowBuilder> OverflowBuilder::WithLocation(PageNumber loc)
{
  if (loc == NULL_PAGE) {
    throw std::domain_error("GetLocation cannot be zero.");
  }

  this->location = loc;
  return this->shared_from_this();
}

std::shared_ptr<OverflowBuilder> OverflowBuilder::WithNext(PageNumber page_number)
{
  this->next = page_number;
  return this->shared_from_this();
}

std::shared_ptr<OverflowBuilder> OverflowBuilder::WithData(DynamicArray<byte>&& bytes)
{
  if (bytes.size() > this->MaxDataSize()) {
    std::stringstream stream;
    stream << "Data size too large; can fit at most " << +this->MaxDataSize() << " bytes.";
    throw std::length_error(stream.str());
  }

  this->data = std::move(bytes);
  this->data_size = static_cast<uint16_t>(this->data.size());
  return this->shared_from_this();
}

std::shared_ptr<OverflowBuilder> OverflowBuilder::WithData(DynamicArray<byte>&& bytes, uint16_t size)
{
  if (bytes.size() > this->MaxDataSize()) {
    std::stringstream stream;
    stream << "Data size too large; can fit at most " << +this->MaxDataSize() << " bytes.";
    throw std::length_error(stream.str());
  }
  else if (bytes.size() < size) {
    throw std::length_error(
        "Having bytes.size() being smaller than the indicated size will result in data corruption.");
  }

  this->data = std::move(bytes);
  this->data_size = size;
  return this->shared_from_this();
}

}
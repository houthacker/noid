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

static const uint8_t PAYLOAD_SIZE_OFFSET = 0;
static const uint8_t NEXT_OVERFLOW_PAGE_OFFSET = 2;
static const uint8_t PAYLOAD_OFFSET = Overflow::HEADER_SIZE;

static DynamicArray<byte>& Validate(DynamicArray<byte>& data, uint16_t page_size)
{
  if (data.size() < page_size) {
    throw std::invalid_argument("Given raw data has an invalid page size.");
  }

  return data;
}

Overflow::Overflow(uint16_t payload_size, PageNumber next, DynamicArray<byte>&& data, uint16_t page_size)
    :payload_size(payload_size), next(next), data(std::move(data)), page_size(page_size) { }

std::unique_ptr<OverflowBuilder> Overflow::NewBuilder(uint16_t page_size)
{
  return std::unique_ptr<OverflowBuilder>(new OverflowBuilder(page_size));
}

std::unique_ptr<OverflowBuilder> Overflow::NewBuilder(const Overflow& base)
{
  return std::unique_ptr<OverflowBuilder>(new OverflowBuilder(base));
}

std::unique_ptr<OverflowBuilder> Overflow::NewBuilder(DynamicArray<byte>&& base)
{
  return std::unique_ptr<OverflowBuilder>(
      new OverflowBuilder(std::move(Validate(base, safe_cast<uint16_t>(base.size())))));
}

uint16_t Overflow::GetPayloadSize() const
{
  return this->payload_size;
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

  write_le_uint16<byte>(bytes, PAYLOAD_SIZE_OFFSET, this->payload_size);
  write_le_uint32<byte>(bytes, NEXT_OVERFLOW_PAGE_OFFSET, this->next);
  write_contiguous_container<byte>(bytes, PAYLOAD_OFFSET, this->data);

  return bytes;
}

/*** OverflowBuilder ***/


OverflowBuilder::OverflowBuilder(uint16_t page_size)
    :page_size(page_size) { }

OverflowBuilder::OverflowBuilder(const Overflow& base)
    :page_size(base.page_size), payload_size(base.payload_size), next(base.next), data(base.data) { }

OverflowBuilder::OverflowBuilder(DynamicArray<byte>&& base)
    :page_size(static_cast<decltype(page_size)>(base.size())),
     payload_size(read_le_uint16<byte>(base, PAYLOAD_SIZE_OFFSET)),
     next(read_le_uint32<byte>(base, NEXT_OVERFLOW_PAGE_OFFSET)),
     data(page_size - PAYLOAD_OFFSET)
{
  write_contiguous_container<byte>(this->data, 0, base, PAYLOAD_OFFSET);
}

std::unique_ptr<const Overflow> OverflowBuilder::Build()
{
  if (this->page_size == 0) {
    throw std::domain_error("Cannot build Overflow: page_size is zero.");
  }
  else if (this->payload_size == 0) {
    throw std::domain_error("Cannot build Overflow: payload_size is zero.");
  }

  return std::unique_ptr<const Overflow>(new Overflow(this->payload_size, this->next, std::move(this->data), this->page_size));
}

DynamicArray<byte>::size_type OverflowBuilder::MaxDataSize() const
{
  if (this->page_size < Overflow::HEADER_SIZE) {
    return 0;
  }

  return this->page_size - Overflow::HEADER_SIZE;
}

OverflowBuilder& OverflowBuilder::WithPayloadSize(uint16_t size)
{
  if (size > this->page_size - PAYLOAD_OFFSET) {
    std::stringstream stream;
    stream << "Payload size too large; can fit at most " << +(this->page_size - PAYLOAD_OFFSET) << " bytes.";
    throw std::length_error(stream.str());
  }

  this->payload_size = size;
  return *this;
}

OverflowBuilder& OverflowBuilder::WithNext(PageNumber page_number)
{
  this->next = page_number;
  return *this;
}

OverflowBuilder& OverflowBuilder::WithData(DynamicArray<byte>&& payload)
{
  if (payload.size() > this->MaxDataSize()) {
    std::stringstream stream;
    stream << "Data size too large; can fit at most " << +this->MaxDataSize() << " bytes.";
    throw std::length_error(stream.str());
  }

  this->data = std::move(payload);
  return *this;
}

}
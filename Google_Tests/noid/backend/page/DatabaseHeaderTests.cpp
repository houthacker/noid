/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <array>
#include <stdexcept>

#include "backend/Bits.h"
#include "backend/Types.h"
#include "backend/page/DatabaseHeader.h"

using ::testing::ContainerEq;
using namespace noid::backend;

#ifndef NOID_SH_PTR_TO_REF
#include <type_traits>
#define NOID_SH_PTR_TO_REF(x) static_cast<decltype(x)::element_type &>(*(x))
#endif

class DatabaseHeaderFixture : public ::testing::Test {};

TEST_F(DatabaseHeaderFixture, CreateInvalidHeader) {
  std::array<byte, page::DatabaseHeader::BYTE_SIZE> raw = {0};

  EXPECT_THROW(page::DatabaseHeader::NewBuilder(raw)->Build(), std::invalid_argument);
}

TEST_F(DatabaseHeaderFixture, CreateValidHeader) {
  std::array<byte, page::DatabaseHeader::BYTE_SIZE> valid_header = {'n', 'o', 'i', 'd', ' ', 'v', '1', '\0'};
  auto page_size = 4096;
  auto key_size = 16;
  auto signature_offset = 11;
  write_le_uint16(valid_header, 8, page_size);
  write_uint8(valid_header, 10, key_size);
  write_le_uint32(valid_header, signature_offset, fnv1a(valid_header, 0, signature_offset));

  EXPECT_NO_THROW(page::DatabaseHeader::NewBuilder(valid_header)->Build());
}

TEST_F(DatabaseHeaderFixture, CreateWithDefaultValues) {
  auto page = page::DatabaseHeader::NewBuilder()->Build();
  EXPECT_EQ(page->GetPageSize(), 4096);
  EXPECT_EQ(page->GetKeySize(), 16);
  EXPECT_EQ(page->GetFirstFreelistPage(), 0);

  std::array<byte, page::DatabaseHeader::BYTE_SIZE> expected_bytes = {'n', 'o', 'i', 'd', ' ', 'v', '1', '\0'};
  write_le_uint16(expected_bytes, 8, 4096);
  write_uint8(expected_bytes, 10, 16);
  write_le_uint32(expected_bytes, 11, fnv1a(expected_bytes, 0, 11));

  EXPECT_THAT(page->GetBytes(), ContainerEq(expected_bytes));
}

TEST_F(DatabaseHeaderFixture, UpdateFreelistPage) {
  auto original = page::DatabaseHeader::NewBuilder()->Build();
  auto updated = page::DatabaseHeader::NewBuilder(original)->WithFirstFreeListPage(1337).Build();

  EXPECT_EQ(original->GetFirstFreelistPage(), 0);
  EXPECT_EQ(updated->GetFirstFreelistPage(), 1337);
  EXPECT_NE(NOID_SH_PTR_TO_REF(original), NOID_SH_PTR_TO_REF(updated));
}
/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "backend/Types.h"
#include "backend/Bits.h"

using ::testing::ContainerEq;
using namespace noid::backend;

class BitsFixture : public ::testing::Test {};

TEST_F(BitsFixture, safe_round_to_next_power_of_2) {
  EXPECT_EQ(safe_round_to_next_power_of_2(32768), 32768);
  EXPECT_EQ(safe_round_to_next_power_of_2(32769), 32768);
  EXPECT_EQ(safe_round_to_next_power_of_2(0), 2);
  EXPECT_EQ(safe_round_to_next_power_of_2(1), 2);
  EXPECT_EQ(safe_round_to_next_power_of_2(2), 2);
}

TEST_F(BitsFixture, safe_next_multiple_of_8) {
  EXPECT_EQ(safe_next_multiple_of_8(0), 8);
  EXPECT_EQ(safe_next_multiple_of_8(1), 8);
  EXPECT_EQ(safe_next_multiple_of_8(8), 8);
  EXPECT_EQ(safe_next_multiple_of_8(9), 16);
  EXPECT_EQ(safe_next_multiple_of_8(std::numeric_limits<uint8_t>::max() - 1), std::numeric_limits<uint8_t>::max());
  EXPECT_EQ(safe_next_multiple_of_8(std::numeric_limits<uint8_t>::max()), std::numeric_limits<uint8_t>::max());
}

TEST_F(BitsFixture, read_uint8) {
  std::array<byte, 3> array = {0, 1, 0};

  EXPECT_EQ(read_uint8(array, 0), 0);
  EXPECT_EQ(read_uint8(array, 1), 1);
  EXPECT_EQ(read_uint8(array, 2), 0);
  EXPECT_THROW(read_uint8(array, 3), std::out_of_range);
}

TEST_F(BitsFixture, write_uint8) {
  std::array<byte, 3> array = {0};
  EXPECT_THAT(write_uint8(array, 0, 0), ContainerEq(std::array<byte, 3>{0}));

  array = {0}; EXPECT_THAT(write_uint8(array, 1, 1), ContainerEq(std::array<byte, 3>{0, 1, 0}));
  array = {0}; EXPECT_THAT(write_uint8(array, 2, 0), ContainerEq(std::array<byte, 3>{0}));
  array = {0}; EXPECT_THROW(write_uint8(array, 3, 1), std::out_of_range);
}

TEST_F(BitsFixture, read_le_uint16) {
  std::array<byte, 5> array = {0, 0, 1, 0, 0};

  EXPECT_EQ(read_le_uint16(array, 0), 0);
  EXPECT_EQ(read_le_uint16(array, 1), 256);
  EXPECT_EQ(read_le_uint16(array, 2), 1);
  EXPECT_EQ(read_le_uint16(array, 3), 0);
  EXPECT_THROW(read_le_uint16(array, 4), std::out_of_range);
  EXPECT_THROW(read_le_uint16(array, 5), std::out_of_range);
}

TEST_F(BitsFixture, write_le_uint16) {
  std::array<byte, 5> array = {0};
  EXPECT_THAT(write_le_uint16(array, 0, 0),    ContainerEq(std::array<byte, 5>{0}));

  array = {0}; EXPECT_THAT(write_le_uint16(array, 1, 256),  ContainerEq(std::array<byte, 5>{0, 0, 1, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint16(array, 2, 1),    ContainerEq(std::array<byte, 5>{0, 0, 1, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint16(array, 3, 0),    ContainerEq(std::array<byte, 5>{0}));
  array = {0}; EXPECT_THROW(write_le_uint16(array, 4, 1),   std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint16(array, 5, 1),   std::out_of_range);
}

TEST_F(BitsFixture, read_le_uint32) {
  std::array<byte, 9> array = {0, 0, 0, 0, 1, 0, 0, 0, 0};

  EXPECT_EQ(read_le_uint32(array, 0), 0);
  EXPECT_EQ(read_le_uint32(array, 1), (uint32_t)pow(2, 24));
  EXPECT_EQ(read_le_uint32(array, 2), (uint32_t)pow(2, 16));
  EXPECT_EQ(read_le_uint32(array, 3), (uint32_t)pow(2, 8));
  EXPECT_EQ(read_le_uint32(array, 4), 1);
  EXPECT_EQ(read_le_uint32(array, 5), 0);
  EXPECT_THROW(read_le_uint32(array, 6), std::out_of_range);
  EXPECT_THROW(read_le_uint32(array, 7), std::out_of_range);
  EXPECT_THROW(read_le_uint32(array, 8), std::out_of_range);
  EXPECT_THROW(read_le_uint32(array, 9), std::out_of_range);
}

TEST_F(BitsFixture, write_le_uint32) {
  std::array<byte, 9> array = {0};

  array = {0}; EXPECT_THAT(write_le_uint32(array, 0,                   0),  ContainerEq(std::array<byte, 9>{0}));
  array = {0}; EXPECT_THAT(write_le_uint32(array, 1, (uint32_t)pow(2, 24)), ContainerEq(std::array<byte, 9>{0, 0, 0, 0, 1, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint32(array, 2, (uint32_t)pow(2, 16)), ContainerEq(std::array<byte, 9>{0, 0, 0, 0, 1, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint32(array, 3, (uint32_t)pow(2,  8)), ContainerEq(std::array<byte, 9>{0, 0, 0, 0, 1, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint32(array, 4,                   1),  ContainerEq(std::array<byte, 9>{0, 0, 0, 0, 1, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint32(array, 5,                   0),  ContainerEq(std::array<byte, 9>{0}));
  array = {0}; EXPECT_THROW(write_le_uint32(array, 6, 1),                   std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint32(array, 7, 1),                   std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint32(array, 8, 1),                   std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint32(array, 9, 1),                   std::out_of_range);

  array = {0}; EXPECT_EQ(read_le_uint32(write_le_uint32(array, 0, 3336502024), 0), 3336502024);
}

TEST_F(BitsFixture, read_le_uint64) {
  std::array<byte, 17> array = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};

  EXPECT_EQ(read_le_uint64(array, 0), 0);
  EXPECT_EQ(read_le_uint64(array, 1), (uint64_t)pow(2, 56));
  EXPECT_EQ(read_le_uint64(array, 2), (uint64_t)pow(2, 48));
  EXPECT_EQ(read_le_uint64(array, 3), (uint64_t)pow(2, 40));
  EXPECT_EQ(read_le_uint64(array, 4), (uint64_t)pow(2, 32));
  EXPECT_EQ(read_le_uint64(array, 5), (uint64_t)pow(2, 24));
  EXPECT_EQ(read_le_uint64(array, 6), (uint64_t)pow(2, 16));
  EXPECT_EQ(read_le_uint64(array, 7), (uint64_t)pow(2, 8));
  EXPECT_EQ(read_le_uint64(array, 8), 1);
  EXPECT_EQ(read_le_uint64(array, 9), 0);
  EXPECT_THROW(read_le_uint64(array, 10), std::out_of_range);
  EXPECT_THROW(read_le_uint64(array, 11), std::out_of_range);
  EXPECT_THROW(read_le_uint64(array, 12), std::out_of_range);
  EXPECT_THROW(read_le_uint64(array, 13), std::out_of_range);
  EXPECT_THROW(read_le_uint64(array, 14), std::out_of_range);
  EXPECT_THROW(read_le_uint64(array, 15), std::out_of_range);
  EXPECT_THROW(read_le_uint64(array, 16), std::out_of_range);
  EXPECT_THROW(read_le_uint64(array, 17), std::out_of_range);
  EXPECT_THROW(read_le_uint64(array, 18), std::out_of_range);
}

TEST_F(BitsFixture, write_le_uint64) {
  std::array<byte, 17> array = {0};

  array = {0}; EXPECT_THAT(write_le_uint64(array, 0,                   0),  ContainerEq(std::array<byte, 17>{0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 1, (uint64_t)pow(2, 56)), ContainerEq(std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 2, (uint64_t)pow(2, 48)), ContainerEq(std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 3, (uint64_t)pow(2, 40)), ContainerEq(std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 4, (uint64_t)pow(2, 32)), ContainerEq(std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 5, (uint64_t)pow(2, 24)), ContainerEq(std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 6, (uint64_t)pow(2, 16)), ContainerEq(std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 7, (uint64_t)pow(2,  8)), ContainerEq(std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 8,                   1),  ContainerEq(std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}));
  array = {0}; EXPECT_THAT(write_le_uint64(array, 9,                   0),  ContainerEq(std::array<byte, 17>{0}));
  array = {0}; EXPECT_THROW(write_le_uint64(array, 10, 1),                  std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint64(array, 11, 1),                  std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint64(array, 12, 1),                  std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint64(array, 13, 1),                  std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint64(array, 14, 1),                  std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint64(array, 15, 1),                  std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint64(array, 16, 1),                  std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint64(array, 17, 1),                  std::out_of_range);
  array = {0}; EXPECT_THROW(write_le_uint64(array, 18, 1),                  std::out_of_range);
}

TEST_F(BitsFixture, fnv1a) {
  std::array<byte, 19> valid_header = {'n', 'o', 'i', 'd', ' ', 'v', '1', '\0'};
  write_le_uint16(valid_header, 8, 4096);
  write_uint8(valid_header, 10, 16);
  write_le_uint32(valid_header, 15, fnv1a(valid_header, 0, 15) /* 3336502024 */);

  uint32_t expected = 1150295512;
  EXPECT_EQ(fnv1a(valid_header, 0, 15), expected);
}
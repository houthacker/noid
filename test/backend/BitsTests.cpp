/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "backend/Types.h"
#include "backend/Bits.h"

using namespace noid::backend;

TEST_CASE("safe_round_to_next_power_of_2()") {
  REQUIRE(safe_round_to_next_power_of_2(32768) == 32768);
  REQUIRE(safe_round_to_next_power_of_2(32769) == 32768);
  REQUIRE(safe_round_to_next_power_of_2(0) == 2);
  REQUIRE(safe_round_to_next_power_of_2(1) == 2);
  REQUIRE(safe_round_to_next_power_of_2(2) == 2);
}

TEST_CASE("safe_next_multiple_of_8()") {
  REQUIRE(safe_next_multiple_of_8(0) == 8);
  REQUIRE(safe_next_multiple_of_8(1) == 8);
  REQUIRE(safe_next_multiple_of_8(8) == 8);
  REQUIRE(safe_next_multiple_of_8(9) == 16);
  REQUIRE(safe_next_multiple_of_8(std::numeric_limits<uint8_t>::max() - 1) == std::numeric_limits<uint8_t>::max());
  REQUIRE(safe_next_multiple_of_8(std::numeric_limits<uint8_t>::max()) == std::numeric_limits<uint8_t>::max());
}

TEST_CASE("read_uint8()") {
  std::array<byte, 3> array = {0, 1, 0};

  REQUIRE(read_uint8<byte>(array, 0) == 0);
  REQUIRE(read_uint8<byte>(array, 1) == 1);
  REQUIRE(read_uint8<byte>(array, 2) == 0);
  REQUIRE_THROWS_AS(read_uint8<byte>(array, 3), std::out_of_range);
}

TEST_CASE("write_uint8() with std::array") {
  std::array<byte, 3> array = {0};
  REQUIRE(write_uint8<byte>(array, 0, 0) == std::array<byte, 3>{0});

  array = {0}; REQUIRE(write_uint8<byte>(array, 1, 1) == std::array<byte, 3>{0, 1, 0});
  array = {0}; REQUIRE(write_uint8<byte>(array, 2, 0) == std::array<byte, 3>{0});
  array = {0}; REQUIRE_THROWS_AS(write_uint8<byte>(array, 3, 1), std::out_of_range);
}

TEST_CASE("write_uint8() with std::vector") {
  std::vector<byte> vector = {0, 0, 0};
  REQUIRE(write_uint8<byte>(vector, 0, 0) == std::vector<byte>{0, 0, 0});

  vector = {0, 0, 0}; REQUIRE(write_uint8<byte>(vector, 1, 1) == std::vector<byte>{0, 1, 0});
  vector = {0, 0, 0}; REQUIRE(write_uint8<byte>(vector, 2, 0) == std::vector<byte>{0, 0, 0});
  vector = {0, 0, 0}; REQUIRE_NOTHROW(write_uint8<byte>(vector, 3, 1));
}

TEST_CASE("read_le_uint16()") {
  std::array<byte, 5> array = {0, 0, 1, 0, 0};

  REQUIRE(read_le_uint16<byte>(array, 0) == 0);
  REQUIRE(read_le_uint16<byte>(array, 1) == 256);
  REQUIRE(read_le_uint16<byte>(array, 2) == 1);
  REQUIRE(read_le_uint16<byte>(array, 3) == 0);
  REQUIRE_THROWS_AS(read_le_uint16<byte>(array, 4), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint16<byte>(array, 5), std::out_of_range);
}

TEST_CASE("write_le_uint16() with std::array") {
  std::array<byte, 5> array = {0};
  REQUIRE(write_le_uint16<byte>(array, 0, 0) == std::array<byte, 5>{0});

  array = {0}; REQUIRE(write_le_uint16<byte>(array, 1, 256) == std::array<byte, 5>{0, 0, 1, 0, 0});
  array = {0}; REQUIRE(write_le_uint16<byte>(array, 2, 1) == std::array<byte, 5>{0, 0, 1, 0, 0});
  array = {0}; REQUIRE(write_le_uint16<byte>(array, 3, 0) == std::array<byte, 5>{0});
  array = {0}; REQUIRE_THROWS_AS(write_le_uint16<byte>(array, 4, 1),   std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint16<byte>(array, 5, 1),   std::out_of_range);
}

TEST_CASE("write_le_uint16() with std::vector") {
  std::vector<byte> vector(5);
  REQUIRE(write_le_uint16<byte>(vector, 0, 0) == std::vector<byte>(5));

  vector = std::vector<byte>(5); REQUIRE(write_le_uint16<byte>(vector, 1, 256) == std::vector<byte>{0, 0, 1, 0, 0});
  vector = std::vector<byte>(5); REQUIRE(write_le_uint16<byte>(vector, 2, 1) == std::vector<byte>{0, 0, 1, 0, 0});
  vector = std::vector<byte>(5); REQUIRE(write_le_uint16<byte>(vector, 3, 0) == std::vector<byte>(5));
  vector = std::vector<byte>(5); REQUIRE_NOTHROW(write_le_uint16<byte>(vector, 4, 1));
  vector = std::vector<byte>(5); REQUIRE_NOTHROW(write_le_uint16<byte>(vector, 5, 1));
}

TEST_CASE("read_le_uint32()") {
  std::array<byte, 9> array = {0, 0, 0, 0, 1, 0, 0, 0, 0};

  REQUIRE(read_le_uint32<byte>(array, 0) == 0);
  REQUIRE(read_le_uint32<byte>(array, 1) == (uint32_t)pow(2, 24));
  REQUIRE(read_le_uint32<byte>(array, 2) == (uint32_t)pow(2, 16));
  REQUIRE(read_le_uint32<byte>(array, 3) == (uint32_t)pow(2, 8));
  REQUIRE(read_le_uint32<byte>(array, 4) == 1);
  REQUIRE(read_le_uint32<byte>(array, 5) == 0);
  REQUIRE_THROWS_AS(read_le_uint32<byte>(array, 6), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint32<byte>(array, 7), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint32<byte>(array, 8), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint32<byte>(array, 9), std::out_of_range);
}

TEST_CASE("write_le_uint32() with std::array") {
  std::array<byte, 9> array = {0};

  array = {0}; REQUIRE(write_le_uint32<byte>(array, 0,                   0)   == std::array<byte, 9>{0});
  array = {0}; REQUIRE(write_le_uint32<byte>(array, 1, (uint32_t)pow(2, 24))  == std::array<byte, 9>{0, 0, 0, 0, 1, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint32<byte>(array, 2, (uint32_t)pow(2, 16))  == std::array<byte, 9>{0, 0, 0, 0, 1, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint32<byte>(array, 3, (uint32_t)pow(2,  8))  == std::array<byte, 9>{0, 0, 0, 0, 1, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint32<byte>(array, 4,                   1)   == std::array<byte, 9>{0, 0, 0, 0, 1, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint32<byte>(array, 5,                   0)   == std::array<byte, 9>{0});
  array = {0}; REQUIRE_THROWS_AS(write_le_uint32<byte>(array, 6, 1),                   std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint32<byte>(array, 7, 1),                   std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint32<byte>(array, 8, 1),                   std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint32<byte>(array, 9, 1),                   std::out_of_range);

  array = {0}; REQUIRE(read_le_uint32<byte>(write_le_uint32<byte>(array, 0, 3336502024), 0)   == 3336502024);
}

TEST_CASE("write_le_uint32() with std::vector") {
  std::vector<byte> vector(9);

  vector = std::vector<byte>(9); REQUIRE(write_le_uint32<byte>(vector, 0, 0)   == std::vector<byte>(9));
  vector = std::vector<byte>(9); REQUIRE(write_le_uint32<byte>(vector, 1, (uint32_t)pow(2, 24))  == std::vector<byte>{0, 0, 0, 0, 1, 0, 0, 0, 0});
  vector = std::vector<byte>(9); REQUIRE(write_le_uint32<byte>(vector, 2, (uint32_t)pow(2, 16))  == std::vector<byte>{0, 0, 0, 0, 1, 0, 0, 0, 0});
  vector = std::vector<byte>(9); REQUIRE(write_le_uint32<byte>(vector, 3, (uint32_t)pow(2, 8))  == std::vector<byte>{0, 0, 0, 0, 1, 0, 0, 0, 0});
  vector = std::vector<byte>(9); REQUIRE(write_le_uint32<byte>(vector, 4, 1)   == std::vector<byte>{0, 0, 0, 0, 1, 0, 0, 0, 0});
  vector = std::vector<byte>(9); REQUIRE(write_le_uint32<byte>(vector, 5, 0)   == std::vector<byte>(9));
  vector = std::vector<byte>(9); REQUIRE_NOTHROW(write_le_uint32<byte>(vector, 6, 1));
  vector = std::vector<byte>(9); REQUIRE_NOTHROW(write_le_uint32<byte>(vector, 7, 1));
  vector = std::vector<byte>(9); REQUIRE_NOTHROW(write_le_uint32<byte>(vector, 8, 1));
  vector = std::vector<byte>(9); REQUIRE_NOTHROW(write_le_uint32<byte>(vector, 9, 1));

  vector = std::vector<byte>(9); REQUIRE(read_le_uint32<byte>(write_le_uint32<byte>(vector, 0, 3336502024), 0)   == 3336502024);
}

TEST_CASE("read_le_uint64()") {
  std::array<byte, 17> array = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
  
  REQUIRE(read_le_uint64<byte>(array, 0) == 0);
  REQUIRE(read_le_uint64<byte>(array, 1) == (uint64_t)pow(2, 56));
  REQUIRE(read_le_uint64<byte>(array, 2) == (uint64_t)pow(2, 48));
  REQUIRE(read_le_uint64<byte>(array, 3) == (uint64_t)pow(2, 40));
  REQUIRE(read_le_uint64<byte>(array, 4) == (uint64_t)pow(2, 32));
  REQUIRE(read_le_uint64<byte>(array, 5) == (uint64_t)pow(2, 24));
  REQUIRE(read_le_uint64<byte>(array, 6) == (uint64_t)pow(2, 16));
  REQUIRE(read_le_uint64<byte>(array, 7) == (uint64_t)pow(2, 8));
  REQUIRE(read_le_uint64<byte>(array, 8) == 1);
  REQUIRE(read_le_uint64<byte>(array, 9) == 0);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 10), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 11), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 12), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 13), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 14), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 15), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 16), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 17), std::out_of_range);
  REQUIRE_THROWS_AS(read_le_uint64<byte>(array, 18), std::out_of_range);
}

TEST_CASE("write_le_uint64() with std::array") {
  std::array<byte, 17> array = {0};
  
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 0,                   0) == std::array<byte, 17>{0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 1, (uint64_t)pow(2, 56)) == std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 2, (uint64_t)pow(2, 48)) == std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 3, (uint64_t)pow(2, 40)) == std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 4, (uint64_t)pow(2, 32)) == std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 5, (uint64_t)pow(2, 24)) == std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 6, (uint64_t)pow(2, 16)) == std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 7, (uint64_t)pow(2,  8)) == std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 8,                   1) == std::array<byte, 17>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  array = {0}; REQUIRE(write_le_uint64<byte>(array, 9,                   0) == std::array<byte, 17>{0});
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 10, 1),                  std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 11, 1),                  std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 12, 1),                  std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 13, 1),                  std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 14, 1),                  std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 15, 1),                  std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 16, 1),                  std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 17, 1),                  std::out_of_range);
  array = {0}; REQUIRE_THROWS_AS(write_le_uint64<byte>(array, 18, 1),                  std::out_of_range);
}

TEST_CASE("write_le_uint64() with std::vector") {
  std::vector<byte> vector(17);

  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 0, 0) == std::vector<byte>(17));
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 1, (uint64_t)pow(2, 56)) == std::vector<byte>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 2, (uint64_t)pow(2, 48)) == std::vector<byte>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 3, (uint64_t)pow(2, 40)) == std::vector<byte>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 4, (uint64_t)pow(2, 32)) == std::vector<byte>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 5, (uint64_t)pow(2, 24)) == std::vector<byte>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 6, (uint64_t)pow(2, 16)) == std::vector<byte>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 7, (uint64_t)pow(2, 8)) == std::vector<byte>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 8, 1) == std::vector<byte>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  vector = std::vector<byte>(17); REQUIRE(write_le_uint64<byte>(vector, 9, 0) == std::vector<byte>(17));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 10, 1));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 11, 1));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 12, 1));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 13, 1));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 14, 1));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 15, 1));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 16, 1));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 17, 1));
  vector = std::vector<byte>(17); REQUIRE_NOTHROW(write_le_uint64<byte>(vector, 18, 1));
}

TEST_CASE("fnv1a()") {
  std::array<byte, 19> valid_header = {'n', 'o', 'i', 'd', ' ', 'v', '1', '\0'};
  write_le_uint16<byte>(valid_header, 8, 4096);
  write_uint8<byte>(valid_header, 10, 16);
  write_le_uint32<byte>(valid_header, 15, fnv1a<byte>(valid_header, 0, 15) /* 3336502024 */);

  uint32_t expected = 1150295512;
  REQUIRE(fnv1a<byte>(valid_header, 0, 15) == expected);
}
/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include "backend/MD5.h"
#include <string>

using namespace noid::backend;

TEST_CASE("Create MD5 hash of empty string") {
  auto empty_string = std::string("");
  auto md5 = MD5::Digest(empty_string);

  auto expected = std::string("d41d8cd98f00b204e9800998ecf8427e");
  REQUIRE(md5.ToHexString() == expected);
}

TEST_CASE("Create MD5 hash of 'The quick brown fox...'") {
  auto empty_string = std::string("The quick brown fox jumps over the lazy dog");
  auto md5 = MD5::Digest(empty_string);

  auto expected = std::string("9e107d9d372bb6826bd81d3542a419d6");
  REQUIRE(md5.ToHexString() == expected);
}
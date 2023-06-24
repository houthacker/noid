/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>
#include <stdexcept>

#include "backend/Bits.h"
#include "backend/Types.h"
#include "backend/page/Overflow.h"

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("Create a valid Overflow page")
{
  auto overflow = Overflow::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithData({1, 2, 3, 4, 5, 0, 0, 0}, 5)
      ->WithNext(2)
      ->Build();

  auto expected_data = DynamicArray<byte>(DEFAULT_PAGE_SIZE - Overflow::HEADER_SIZE);
  write_container<byte>(expected_data, 0, DynamicArray<byte>{1, 2, 3, 4, 5});

  REQUIRE(overflow->GetData() == expected_data);
  REQUIRE(overflow->GetDataSize() == 5);
  REQUIRE(overflow->GetNext() == 2);
}

TEST_CASE("Create an Overflow page based on another")
{
  auto base = Overflow::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithData({1, 2, 3, 4, 5, 0, 0, 0}, 5)
      ->WithNext(2)
      ->Build();

  auto overflow = Overflow::NewBuilder(*base)->Build();
  REQUIRE(overflow->GetData() == base->GetData());
  REQUIRE(overflow->GetDataSize() == base->GetDataSize());
  REQUIRE(overflow->GetNext() == base->GetNext());
}

TEST_CASE("Overflow page (de)serialization cycle"){
  auto base = Overflow::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithData({1, 2, 3, 4, 5, 0, 0, 0}, 5)
      ->WithNext(2)
      ->Build();

  auto deserialized = Overflow::NewBuilder(base->ToBytes())->Build();
  REQUIRE(deserialized->GetData() == base->GetData());
  REQUIRE(deserialized->GetDataSize() == base->GetDataSize());
  REQUIRE(deserialized->GetNext() == base->GetNext());
}

TEST_CASE("OverflowBuilder edge cases")
{
  // Create an OverflowBuilder with (in)valid page_size
  CHECK_THROWS_AS(Overflow::NewBuilder(Overflow::HEADER_SIZE), std::length_error); // page_size too small
  CHECK_NOTHROW(Overflow::NewBuilder(Overflow::HEADER_SIZE + 1));

  // Create an OverflowBuilder with (in)valid payload size
  auto builder = Overflow::NewBuilder(DEFAULT_PAGE_SIZE);
  CHECK_NOTHROW(builder->WithData({1, 2, 3, 4, 5}));
  CHECK_THROWS_AS(builder->WithData({1, 2, 3, 4}, 5), std::length_error);
  CHECK_THROWS_AS(builder->WithData(DynamicArray<byte>(DEFAULT_PAGE_SIZE)), std::length_error);
}
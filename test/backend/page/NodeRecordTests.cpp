/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include "backend/Types.h"
#include "backend/page/NodeRecord.h"

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("Building a NodeRecord without setting any field yields all-zero fields")
{
  auto record = NodeRecord::NewBuilder()->Build();

  REQUIRE(record.GetKey() == SearchKey{0});
  REQUIRE(record.GetOverflowPage() == NULL_PAGE);
  REQUIRE(record.GetPayload() == std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>{0});
  REQUIRE(record.GetInlineIndicator() == 0);

}

TEST_CASE("Build a NodeRecord with inline data")
{
  auto record = NodeRecord::NewBuilder()
      ->WithSearchKey({1})
      ->WithInlinePayload({1, 2, 3, 4, 5}, 5)
      ->Build();

  REQUIRE(record.GetKey() == SearchKey{1});
  REQUIRE(record.GetOverflowPage() == NULL_PAGE);
  REQUIRE(record.GetInlineIndicator() == 5);
  REQUIRE(record.GetPayload() == std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>{1, 2, 3, 4, 5, 0, 0});
}

TEST_CASE("Build a NodeRecord with overflow data")
{
  auto record = NodeRecord::NewBuilder()
      ->WithSearchKey({1})
      ->WithOverflowPayload({1, 2, 3}, 2)
      ->Build();

  REQUIRE(record.GetKey() == SearchKey{1});
  REQUIRE(record.GetOverflowPage() == 2);
  REQUIRE(record.GetInlineIndicator() == 0);
  REQUIRE(record.GetPayload() == std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>{1, 2, 3, 2, 0, 0, 0});
}

TEST_CASE("Calculate the amount of overflow pages for a given value")
{
  const auto expect_overflow_none = V{1, 2, 3, 4, 5, 6, 7};
  const auto expect_overflow_one  = V{1, 2, 3, 4, 5, 6, 7, 8};
  const auto expect_overflow_two  = V(DEFAULT_PAGE_SIZE);

  REQUIRE(NodeRecord::CalculateOverflow({}, DEFAULT_PAGE_SIZE) == 0);
  REQUIRE(NodeRecord::CalculateOverflow(expect_overflow_none, DEFAULT_PAGE_SIZE) == 0);
  REQUIRE(NodeRecord::CalculateOverflow(expect_overflow_one, DEFAULT_PAGE_SIZE) == 1);
  REQUIRE(NodeRecord::CalculateOverflow(expect_overflow_two, DEFAULT_PAGE_SIZE) == 2);
}
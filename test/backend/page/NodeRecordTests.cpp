/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

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

TEST_CASE("A NodeRecord with inline data")
{
  auto record = NodeRecord::NewBuilder()
      ->WithSearchKey({1})
      .WithInlinePayload({1, 2, 3, 4, 5}, 5)
      .Build();

  REQUIRE(record.GetKey() == SearchKey{1});
  REQUIRE(record.GetOverflowPage() == NULL_PAGE);
  REQUIRE(record.GetInlineIndicator() == 5);
  REQUIRE(record.GetPayload() == std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>{1, 2, 3, 4, 5, 0, 0});
}
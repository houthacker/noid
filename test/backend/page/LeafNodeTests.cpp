/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>
#include <stdexcept>

#include "backend/page/LeafNode.h"
#include "backend/page/NodeRecord.h"
#include "backend/NoidConfig.h"
#include "backend/Bits.h"
#include "backend/DynamicArray.h"
#include "backend/Types.h"

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("Build a LeafNode with an inline payload")
{
  auto leaf_node = LeafNode::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLeftSibling(1)
      .WithRightSibling(3)
      .WithRecord(NodeRecord::NewBuilder()
          ->WithSearchKey({0})
          .WithInlinePayload({1, 3, 3, 7}, 4)
          .Build())
      .Build();

  REQUIRE(leaf_node->GetLeftSibling() == 1);
  REQUIRE(leaf_node->GetRightSibling() == 3);
  REQUIRE(leaf_node->Size() == 1);

  auto& record = leaf_node->RecordAt(0);
  REQUIRE(record.GetKey() == std::array<byte, FIXED_KEY_SIZE>{0});
  REQUIRE(record.GetInlineIndicator() == 4);
  REQUIRE(record.GetPayload() == std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>{1, 3, 3, 7});

  // No overflow page, so trying to retrieve it throws an exception
  CHECK_THROWS_AS(record.GetOverflowPage(), std::domain_error);
}

TEST_CASE("Build a LeafNode with an overflowing NodeRecord")
{
  auto leaf_node = LeafNode::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithRecord(NodeRecord::NewBuilder()
          ->WithSearchKey({0}).WithOverflowPayload({5, 0, 0}, 1337).Build())
      .Build();

  REQUIRE(leaf_node->GetLeftSibling() == 0);
  REQUIRE(leaf_node->GetRightSibling() == 0);
  REQUIRE(leaf_node->Size() == 1);

  auto& record = leaf_node->RecordAt(0);
  REQUIRE(record.GetKey() == SearchKey{0});
  REQUIRE(record.GetInlineIndicator() == 0);
  REQUIRE(record.GetPayload() == std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>{5, 0, 0, 57, 5, 0, 0});
  REQUIRE(record.GetOverflowPage() == (PageNumber) 1337);
}

TEST_CASE("Build a LeafNode and overflow it")
{
  auto builder = LeafNode::NewBuilder(DEFAULT_PAGE_SIZE);

  uint16_t i = 0;
  for (; !builder->IsFull(); i++) {
    std::array<byte, FIXED_KEY_SIZE> key = {0};
    write_le_uint16<byte>(key, 0, i);

    builder->WithRecord(NodeRecord::NewBuilder()->WithSearchKey(key).WithInlinePayload({1, 3, 3, 7}, 4).Build());
  }

  // Write one more record after the builder indicates it is full. This must cause an exception.
  std::array<byte, FIXED_KEY_SIZE> key = {1};
  CHECK_THROWS_AS(
      builder->WithRecord(NodeRecord::NewBuilder()->WithSearchKey(key).WithInlinePayload({1, 3, 3, 8}, 4).Build()),
      std::overflow_error);
}

TEST_CASE("Build a LeafNode based on another and overwrite a record")
{
  auto base = LeafNode::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLeftSibling(1)
      .WithRightSibling(3)
      .WithRecord(NodeRecord::NewBuilder()->WithSearchKey({0}).WithInlinePayload({1, 3, 3, 7}, 4).Build())
      .Build();

  auto leaf_node = LeafNode::NewBuilder(*base)
      ->WithRecord(NodeRecord::NewBuilder()->WithSearchKey({1}).WithInlinePayload({3, 1, 4, 1, 5}, 5).Build(), 0)
      .Build();
  REQUIRE(leaf_node->GetLeftSibling() == base->GetLeftSibling());
  REQUIRE(leaf_node->GetRightSibling() == base->GetRightSibling());
  REQUIRE_FALSE(leaf_node->RecordAt(0) == base->RecordAt(0));

  auto& record = leaf_node->RecordAt(0);
  REQUIRE(record.GetKey() == SearchKey{1});
  REQUIRE(record.GetInlineIndicator() == 5);
  REQUIRE(record.GetPayload() == std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE>{3, 1, 4, 1, 5});
  CHECK_THROWS_AS(record.GetOverflowPage(), std::domain_error);
}

TEST_CASE("Try building a LeafNode from an invalid raw byte vector")
{
  CHECK_THROWS_AS(LeafNode::NewBuilder(DynamicArray<byte>(4096)), std::invalid_argument);
}

TEST_CASE("Check that a LeafNode contains a given NodeRecord")
{
  auto node = LeafNode::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLeftSibling(1)
      .WithRightSibling(3)
      .WithRecord(NodeRecord::NewBuilder()->WithSearchKey({0}).WithInlinePayload({1, 3, 3, 7}, 4).Build())
      .WithRecord(NodeRecord::NewBuilder()->WithSearchKey({1}).WithInlinePayload({1, 3, 3, 8}, 4).Build())
      .WithRecord(NodeRecord::NewBuilder()->WithSearchKey({2}).WithInlinePayload({1, 3, 3, 9}, 4).Build())
      .Build();

  REQUIRE(node->Contains({0}));
  REQUIRE(node->Contains({1}));
  REQUIRE(node->Contains({2}));
  REQUIRE_FALSE(node->Contains({3}));
}

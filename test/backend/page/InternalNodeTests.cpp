/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>
#include <stdexcept>

#include "backend/page/InternalNode.h"
#include "backend/NoidConfig.h"
#include "backend/Bits.h"
#include "backend/DynamicArray.h"

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("Build an InternalNode")
{
  auto internal_node = InternalNode::NewBuilder()
      ->WithLeftmostChild(1)
      .WithEntry({1, 3, 3, 7}, 2)
      .Build();

  REQUIRE(internal_node->Size() == 1);
  REQUIRE(internal_node->GetLeftmostChild() == 1);
  REQUIRE(internal_node->EntryAt(0) == NodeEntry{{1, 3, 3, 7}, 2});
  CHECK_THROWS_AS(internal_node->EntryAt(internal_node->Size()), std::out_of_range);
}

TEST_CASE("Build an InternalNode based on another")
{
  auto base = InternalNode::NewBuilder()
      ->WithLeftmostChild(1)
      .WithEntry({1, 3, 3, 7}, 2)
      .Build();

  auto internal_node = InternalNode::NewBuilder(*base)
      ->WithEntry({1, 3, 3, 8}, 3)
      .Build();
  REQUIRE(internal_node->Size() == 2);
  REQUIRE(internal_node->GetLeftmostChild() == 1);
  REQUIRE(internal_node->EntryAt(0) == base->EntryAt(0));
  REQUIRE(internal_node->EntryAt(1) == NodeEntry{{1, 3, 3, 8}, 3});
}

TEST_CASE("Build an InternalNode based on another and overwrite some entries")
{
  auto base = InternalNode::NewBuilder()
      ->WithLeftmostChild(1)
      .WithEntry({1, 3, 3, 7}, 2)
      .WithEntry({1, 3, 3, 8}, 3)
      .Build();

  auto internal_node = InternalNode::NewBuilder(*base)
      ->WithLeftmostChild(4)
      .WithEntry({1, 3, 3, 9}, 3, 1)
      .Build();

  REQUIRE(internal_node->Size() == 2);
  REQUIRE(internal_node->GetLeftmostChild() == 4);
  REQUIRE(internal_node->EntryAt(0) == base->EntryAt(0));
  REQUIRE(internal_node->EntryAt(1) == NodeEntry{{1, 3, 3, 9}, 3});
  REQUIRE_FALSE(internal_node->EntryAt(1) == base->EntryAt(1));
}

TEST_CASE("Build an InternalNode based on an invalid raw byte vector")
{
  CHECK_THROWS_AS(InternalNode::NewBuilder(DynamicArray<byte>(4096)), std::invalid_argument);
}

TEST_CASE("Try building an InternalNode that contains too many entries")
{
  auto default_max_entries = 203;
  auto builder = InternalNode::NewBuilder();

  for (auto i = 0; i < default_max_entries; i++) {
    builder->WithEntry({1, 3, 3, static_cast<byte>(i)}, i);
  }

  CHECK_THROWS_AS(builder->WithEntry({1, 3, 3, 204}, 204), std::overflow_error);
}
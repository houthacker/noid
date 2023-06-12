/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include "backend/Algorithm.h"
#include "backend/DynamicArray.h"
#include "backend/page/InternalNode.h" // NodeEntry

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("BinarySearch on a DynamicArray<NodeEntry>") {
  DynamicArray<NodeEntry> data{
    {{1}, 10},
    {{2}, 11},
    {{3}, 12},
    {{4}, 13},
    {{5}, 14}
  };

  REQUIRE(BinarySearch<NodeEntry>(data, 0, data.size(), {0}) == data.size());
  REQUIRE(BinarySearch<NodeEntry>(data, 0, data.size(), {6}) == data.size());

  for (decltype(data.size()) i = 0; i < data.size(); i++) {
    REQUIRE(BinarySearch<NodeEntry>(data, 0, data.size(), data[i].key) == i);
  }
}
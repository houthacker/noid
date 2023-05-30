/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>
#include <stdexcept>

#include "backend/page/TreeHeader.h"
#include "backend/NoidConfig.h"
#include "backend/Bits.h"

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("Build a TreeHeader") {
  auto tree_header = TreeHeader::NewBuilder()
      ->WithTreeType(TreeType::Table)
      .WithRootPageNumber(1337)
      .Build();

  REQUIRE(tree_header->GetTreeType() == TreeType::Table);
  REQUIRE(tree_header->GetRootNodePageNumber() == 1337);
  REQUIRE(tree_header->GetMaxInternalEntries() == 203); // calculated default value
  REQUIRE(tree_header->GetMaxLeafRecords() == 169); // calculated default value
}

TEST_CASE("Build a TreeHeader without designated TreeType") {
  CHECK_THROWS_AS(TreeHeader::NewBuilder()->Build(), std::domain_error);
}

TEST_CASE("Build a TreeHeader based on another TreeHeader") {
  auto base = TreeHeader::NewBuilder()
      ->WithTreeType(TreeType::Index)
      .WithRootPageNumber(1338)
      .Build();

  auto tree_header = TreeHeader::NewBuilder(*base)
      ->WithRootPageNumber(1337)
      .Build();
  REQUIRE(tree_header->GetTreeType() == TreeType::Index);
  REQUIRE(tree_header->GetRootNodePageNumber() == 1337);
  REQUIRE(tree_header->GetMaxInternalEntries() == base->GetMaxInternalEntries());
  REQUIRE(tree_header->GetMaxLeafRecords() == base->GetMaxLeafRecords());
}

TEST_CASE("Build a TreeHeader based on another TreeHeader and try to change the TreeType") {
  auto base = TreeHeader::NewBuilder()
      ->WithTreeType(TreeType::Index)
      .WithRootPageNumber(1338)
      .Build();

  CHECK_THROWS_AS(TreeHeader::NewBuilder(*base)->WithTreeType(TreeType::Table).Build(), std::domain_error);
}

TEST_CASE("Build a TreeHeader based on a std::vector<byte>") {
  auto page_size = NoidConfig::Get().vfs_page_size;

  // Try building one with an all-zeroes vector
  CHECK_THROWS_AS(TreeHeader::NewBuilder(std::vector<byte>(page_size))->Build(), std::invalid_argument);

  // Prepare a valid vector as if retrieved from storage
  auto const tree_type = TreeType::Table;
  auto const max_entries = 203;
  auto const max_records = 169;
  auto const root_page = 1337;
  auto base = std::vector<byte>(page_size);
  write_le_uint16<byte>(base, 0, (uint16_t)tree_type);
  write_uint8<byte>(base, sizeof(uint16_t), max_entries);
  write_uint8<byte>(base, sizeof(uint16_t) + sizeof(uint8_t), max_records);
  write_le_uint16<byte>(base, sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t), root_page);

  auto tree_header = TreeHeader::NewBuilder(std::move(base))->Build();
  REQUIRE(tree_header->GetTreeType() == tree_type);
  REQUIRE(tree_header->GetRootNodePageNumber() == root_page);
  REQUIRE(tree_header->GetMaxInternalEntries() == max_entries);
  REQUIRE(tree_header->GetMaxLeafRecords() == max_records);
}

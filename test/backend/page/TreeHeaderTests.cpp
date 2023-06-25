/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>
#include <limits>
#include <stdexcept>

#include "backend/page/TreeHeader.h"
#include "backend/Bits.h"
#include "backend/DynamicArray.h"
#include "backend/Types.h"

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("Build a TreeHeader")
{
  auto tree_header = TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithTreeType(TreeType::Table)
      ->WithRootPageNumber(1337)
      ->WithPageCount(13)
      ->Build();

  REQUIRE(tree_header->GetTreeType() == TreeType::Table);
  REQUIRE(tree_header->GetRoot() == 1337);
  REQUIRE(tree_header->GetPageCount() == 13);
  REQUIRE(tree_header->GetMaxInternalEntries() == 203); // calculated default value
  REQUIRE(tree_header->GetMaxLeafRecords() == 169); // calculated default value
}

TEST_CASE("Build a TreeHeader with default values")
{
  auto tree_header = TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithTreeType(TreeType::Table)
      ->Build();

  REQUIRE(tree_header->GetTreeType() == TreeType::Table);
  REQUIRE(tree_header->GetRoot() == NULL_PAGE);
  REQUIRE(tree_header->GetPageCount() == 1);
  REQUIRE(tree_header->GetMaxInternalEntries() == 203); // calculated default value
  REQUIRE(tree_header->GetMaxLeafRecords() == 169); // calculated default value
}

TEST_CASE("Build a TreeHeader without designated TreeType")
{
  CHECK_THROWS_AS(TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)->Build(), std::domain_error);
}

TEST_CASE("Build a TreeHeader based on another TreeHeader")
{
  auto base = TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithTreeType(TreeType::Index)
      ->WithRootPageNumber(1338)
      ->Build();

  auto tree_header = TreeHeader::NewBuilder(*base)
      ->Build();
  REQUIRE(tree_header->GetTreeType() == base->GetTreeType());
  REQUIRE(tree_header->GetRoot() == base->GetRoot());
  REQUIRE(tree_header->GetMaxInternalEntries() == base->GetMaxInternalEntries());
  REQUIRE(tree_header->GetMaxLeafRecords() == base->GetMaxLeafRecords());
}

TEST_CASE("Build a TreeHeader based on another and update the root page")
{
  auto base = TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithTreeType(TreeType::Index)
      ->WithRootPageNumber(1338)
      ->Build();

  CHECK_NOTHROW(TreeHeader::NewBuilder(*base)->WithRootPageNumber(base->GetRoot() - 1)->Build());
}

TEST_CASE("Build a TreeHeader based on another and try to change the tree type")
{
  auto base = TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithTreeType(TreeType::Index)
      ->WithRootPageNumber(1338)
      ->Build();

  CHECK_THROWS_AS(TreeHeader::NewBuilder(*base)->WithTreeType(TreeType::Table)->Build(), std::domain_error);
}

TEST_CASE("Build a TreeHeader and try to leave the tree type at its default value of TreeType::None")
{
  CHECK_THROWS_AS(TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)->WithRootPageNumber(1337)->Build(), std::domain_error);
}

TEST_CASE("Build a TreeHeader based on another TreeHeader and try to change the TreeType")
{
  auto base = TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithTreeType(TreeType::Index)
      ->WithRootPageNumber(1338)
      ->Build();

  CHECK_THROWS_AS(TreeHeader::NewBuilder(*base)->WithTreeType(TreeType::Table)->Build(), std::domain_error);
}

TEST_CASE("Build a TreeHeader based on a std::vector<byte>")
{

  // Try building one with an all-zeroes vector
  CHECK_THROWS_AS(TreeHeader::NewBuilder(DynamicArray<byte>(DEFAULT_PAGE_SIZE))->Build(), std::invalid_argument);

  // Prepare a valid vector as if retrieved from storage
  auto const tree_type = TreeType::Table;
  auto const max_entries = 203;
  auto const max_records = 169;
  auto const root_page = 1337;
  auto const page_count = 13;
  auto base = DynamicArray<byte>(DEFAULT_PAGE_SIZE);
  write_le_uint16<byte>(base, 0, (uint16_t) tree_type);
  write_le_uint16<byte>(base, sizeof(uint16_t), max_entries);
  write_le_uint16<byte>(base, sizeof(uint16_t) + sizeof(uint16_t), max_records);
  write_le_uint32<byte>(base, sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t), root_page);
  write_le_uint32<byte>(base, sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t), page_count);

  auto tree_header = TreeHeader::NewBuilder(std::move(base))->Build();
  REQUIRE(tree_header->GetTreeType() == tree_type);
  REQUIRE(tree_header->GetRoot() == root_page);
  REQUIRE(tree_header->GetMaxInternalEntries() == max_entries);
  REQUIRE(tree_header->GetMaxLeafRecords() == max_records);
  REQUIRE(tree_header->GetPageCount() == page_count);
}

TEST_CASE("Build a TreeHeader and try to overflow the page count")
{
  CHECK_THROWS_AS(TreeHeader::NewBuilder(DEFAULT_PAGE_SIZE)
    ->WithPageCount(1)
    ->IncrementPageCount(std::numeric_limits<uint32_t>::max()), std::overflow_error);
}
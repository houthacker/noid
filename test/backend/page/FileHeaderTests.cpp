/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include "backend/page/FileHeader.h"

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("Build a FileHeader")
{
  auto header = FileHeader::NewBuilder()
      ->WithFirstTreeHeaderPage(1)
      .WithFirstFreeListPage(2)
      .WithKeySize(23)    // is rounded up to the next multiple of 8
      .WithPageSize(1023) // is rounded up to the next power of two
      .Build();

  REQUIRE(header->GetPageSize() == 1024);
  REQUIRE(header->GetKeySize() == 24);
  REQUIRE(header->GetFirstTreeHeaderPage() == 1);
  REQUIRE(header->GetFirstFreelistPage() == 2);
  REQUIRE(header->GetChecksum() == 0x98a4eae7);
}

TEST_CASE("Create a FileHeader with default values")
{
  auto header = FileHeader::NewBuilder()->Build();

  REQUIRE(header->GetPageSize() == DEFAULT_PAGE_SIZE);
  REQUIRE(header->GetKeySize() == FIXED_KEY_SIZE);
  REQUIRE(header->GetFirstTreeHeaderPage() == 0);
  REQUIRE(header->GetFirstFreelistPage() == 0);
  REQUIRE(header->GetChecksum() == 0xa60a2358);
}

TEST_CASE("Create a FileHeader based on another instance")
{
  auto base = FileHeader::NewBuilder()->Build();
  auto header = FileHeader::NewBuilder(*base)->Build();

  REQUIRE(header->GetPageSize() == base->GetPageSize());
  REQUIRE(header->GetKeySize() == base->GetKeySize());
  REQUIRE(header->GetFirstTreeHeaderPage() == base->GetFirstTreeHeaderPage());
  REQUIRE(header->GetFirstFreelistPage() == base->GetFirstTreeHeaderPage());
  REQUIRE(header->GetChecksum() == base->GetChecksum());
}

TEST_CASE("Compare DatabaseHeaders by using FileHeader::Equals()")
{
  auto base = FileHeader::NewBuilder()->Build();
  auto expect_equal = FileHeader::NewBuilder(*base)->Build();
  auto expect_not_equal = FileHeader::NewBuilder(*base)->WithFirstTreeHeaderPage(1).Build();

  REQUIRE(base->Equals(*expect_equal));
  REQUIRE_FALSE(base->Equals(*expect_not_equal));
  REQUIRE_FALSE(expect_equal->Equals(*expect_not_equal));
  REQUIRE_FALSE(base->GetChecksum() == expect_not_equal->GetChecksum());
}
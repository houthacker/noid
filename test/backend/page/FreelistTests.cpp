/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include <cstdint>
#include <stdexcept>

#include "backend/DynamicArray.h"
#include "backend/Types.h"
#include "backend/page/Freelist.h"

using namespace noid::backend;
using namespace noid::backend::page;

TEST_CASE("Build a Freelist")
{
  auto freelist = Freelist::NewBuilder(DEFAULT_PAGE_SIZE)->WithPrevious(0)
      ->WithLocation(2)
      ->WithNext(0)
      ->WithFreePage(1337)
      ->WithFreePage(1338)
      ->Build();

  REQUIRE(freelist->GetLocation() == 2);
  REQUIRE(freelist->Previous() == 0);
  REQUIRE(freelist->Next() == 0);
  REQUIRE(freelist->FreePageAt(0) == 1337);
  REQUIRE(freelist->FreePageAt(1) == 1338);
  REQUIRE(freelist->Size() == 2);
}

TEST_CASE("Build a Freelist with default values")
{
  auto freelist = Freelist::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLocation(2)
      ->Build();

  REQUIRE(freelist->GetLocation() == 2);
  REQUIRE(freelist->Previous() == 0);
  REQUIRE(freelist->Next() == 0);
  REQUIRE(freelist->Size() == 0);

  CHECK_THROWS_AS(freelist->FreePageAt(freelist->Size()), std::out_of_range);
}

TEST_CASE("Build a Freelist based on another Freelist")
{
  auto base = Freelist::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLocation(2)
      ->WithPrevious(0)
      ->WithNext(0)
      ->WithFreePage(1337)
      ->Build();

  auto freelist = Freelist::NewBuilder(*base)->WithFreePage(1338)->Build();
  REQUIRE(freelist->GetLocation() == base->GetLocation());
  REQUIRE(freelist->Previous() == base->Previous());
  REQUIRE(freelist->Next() == base->Next());
  REQUIRE(freelist->FreePageAt(0) == base->FreePageAt(0));
  REQUIRE(freelist->FreePageAt(1) == 1338);
  REQUIRE(freelist->Size() == 2);
}

TEST_CASE("Build a Freelist from an invalid raw byte vector")
{
  // Try to create a Freelist based on a vector which has the correct size, but is otherwise empty.
  CHECK_THROWS_AS(Freelist::NewBuilder(DynamicArray<byte>(4096)), std::invalid_argument);
}

TEST_CASE("Build a Freelist that contains exactly the maximum amount of pages")
{
  auto default_max_pages = 1021;
  auto builder = Freelist::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLocation(2);

  for (auto i = 0; i < default_max_pages; i++) {
    CHECK_NOTHROW(builder->WithFreePage(i + 1));
  }

  CHECK_NOTHROW(std::ignore = builder->Build());
}

TEST_CASE("Try building a Freelist that contains too many pages")
{
  auto default_max_pages = 1021;
  auto builder = Freelist::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLocation(2);

  for (auto i = 0; i < default_max_pages; i++) {
    builder->WithFreePage(i + 1);
  }

  CHECK_THROWS_AS(builder->WithFreePage(default_max_pages + 1), std::overflow_error);
}

TEST_CASE("Build a Freelist based on another and overwrite some free page numbers")
{
  auto base = Freelist::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLocation(2)
      ->WithPrevious(0)
      ->WithNext(0)
      ->WithFreePage(1337)
      ->WithFreePage(1338)
      ->Build();

  auto freelist = Freelist::NewBuilder(*base)->WithFreePage(1339, 0)->Build();
  REQUIRE(freelist->GetLocation() == base->GetLocation());
  REQUIRE(freelist->Previous() == base->Previous());
  REQUIRE(freelist->Next() == base->Next());
  REQUIRE(freelist->FreePageAt(0) == 1339);
  REQUIRE(freelist->FreePageAt(1) == base->FreePageAt(1));
  REQUIRE(freelist->Size() == base->Size());
}

TEST_CASE("Freelist::ToBytes() cycle") {
  auto original = Freelist::NewBuilder(DEFAULT_PAGE_SIZE)
      ->WithLocation(2)
      ->WithPrevious(1)
      ->WithNext(3)
      ->WithFreePage(1337)
      ->WithFreePage(1338)
      ->Build();

  auto from_bytes = Freelist::NewBuilder(original->ToBytes())
      ->WithLocation(original->GetLocation())
      ->Build();

  REQUIRE(from_bytes->GetLocation() == original->GetLocation());
  REQUIRE(from_bytes->Previous() == original->Previous());
  REQUIRE(from_bytes->Next() == original->Next());
  REQUIRE(from_bytes->FreePageAt(0) == original->FreePageAt(0));
  REQUIRE(from_bytes->FreePageAt(1) == original->FreePageAt(1));
  REQUIRE(from_bytes->Size() == original->Size());
  REQUIRE(from_bytes->ToBytes() == original->ToBytes());
}
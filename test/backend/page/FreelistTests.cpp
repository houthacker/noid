/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include <stdexcept>
#include "backend/page/Freelist.h"

using namespace noid::backend::page;

TEST_CASE("Build a Freelist") {
  auto freelist = Freelist::NewBuilder()->WithPrevious(0)
      .WithNext(0)
      .WithFreePage(1337)
      .WithFreePage(1338)
      .Build();

  REQUIRE(freelist->Previous() == 0);
  REQUIRE(freelist->Next() == 0);
  REQUIRE(freelist->FreePageAt(0) == 1337);
  REQUIRE(freelist->FreePageAt(1) == 1338);
  REQUIRE(freelist->Size() == 2);
}

TEST_CASE("Build a Freelist with default values") {
  auto freelist = Freelist::NewBuilder()->Build();

  REQUIRE(freelist->Previous() == 0);
  REQUIRE(freelist->Next() == 0);
  REQUIRE(freelist->Size() == 0);

  CHECK_THROWS_AS(freelist->FreePageAt(freelist->Size()), std::out_of_range);
}

TEST_CASE("Build a Freelist based on another Freelist") {
  auto base = Freelist::NewBuilder()->WithPrevious(0)
      .WithNext(0)
      .WithFreePage(1337)
      .Build();

  auto freelist = Freelist::NewBuilder(*base)->WithFreePage(1338).Build();
  REQUIRE(freelist->Previous() == 0);
  REQUIRE(freelist->Next() == 0);
  REQUIRE(freelist->FreePageAt(0) == 1337);
  REQUIRE(freelist->FreePageAt(1) == 1338);
  REQUIRE(freelist->Size() == 2);
}

TEST_CASE("Build a Freelist from an invalid raw byte vector") {
  // Try to create a Freelist based on a vector which has the correct size, but is otherwise empty.
  CHECK_THROWS_AS(Freelist::NewBuilder(std::vector<noid::backend::byte>(4096)), std::invalid_argument);
}

TEST_CASE("Build a Freelist that contains exactly the maximum amount of pages") {
  auto default_max_pages = 1021;
  auto builder = Freelist::NewBuilder();

  for (auto i = 0; i < default_max_pages; i++) {
    CHECK_NOTHROW(builder->WithFreePage(i+1));
  }

  CHECK_NOTHROW(std::ignore = builder->Build());
}

TEST_CASE("Build a Freelist that contains too many pages") {
  auto default_max_pages = 1021;
  auto builder = Freelist::NewBuilder();


  for (auto i = 0; i < default_max_pages; i++) {
    builder->WithFreePage(i+1);
  }

  CHECK_THROWS_AS(builder->WithFreePage(1022), std::overflow_error);
}

TEST_CASE("Build a Freelist based on another and overwrite some free page numbers") {
  auto base = Freelist::NewBuilder()->WithPrevious(0)
      .WithNext(0)
      .WithFreePage(1337)
      .WithFreePage(1338)
      .Build();

  auto freelist = Freelist::NewBuilder(*base)->WithFreePage(1339, 0).Build();
  REQUIRE(freelist->Previous() == 0);
  REQUIRE(freelist->Next() == 0);
  REQUIRE(freelist->FreePageAt(0) == 1339);
  REQUIRE(freelist->FreePageAt(1) == 1338);
  REQUIRE(freelist->Size() == 2);
}
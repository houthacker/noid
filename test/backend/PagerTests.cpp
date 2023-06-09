/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>
#include <memory>

#include "backend/Pager.h"
#include "backend/concurrent/IntentAwareMutex.h"
#include "backend/vfs/unix/UnixFile.h"
#include "backend/vfs/unix/UnixVFS.h"
#include "backend/concurrent/unix/UnixFileLock.h"
#include "backend/concurrent/unix/UnixSharedFileLock.h"

#include "backend/page/Freelist.h"

using namespace noid::backend;
using namespace noid::backend::vfs;

using UnixPager = Pager<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>>;

TEST_CASE("A newly opened database file is initialized") {
  auto vfs = std::make_unique<UnixVFS>();
  auto file =  vfs->CreateTempFile();
  auto pager = UnixPager::Open(std::move(file));
  auto header = pager->ReadFileHeader();

  REQUIRE(header->GetPageSize() == DEFAULT_PAGE_SIZE);
  REQUIRE(header->GetKeySize() == FIXED_KEY_SIZE);
  REQUIRE(header->GetFirstFreelistPage() == NO_PAGE);
  REQUIRE(header->GetFirstTreeHeaderPage() == NO_PAGE);
  REQUIRE(header->GetChecksum() == 0xa60a2358);
}

TEST_CASE("StorePage-RetrievePage cycle") {
  auto vfs = std::make_unique<UnixVFS>();
  auto file =  vfs->CreateTempFile();
  auto pager = UnixPager::Open(std::move(file));
  PageNumber page_location = 2;

  auto freelist = Freelist::NewBuilder()->WithPrevious(1)
      .WithNext(3)
      .WithFreePage(1337)
      .WithFreePage(1338)
      .Build();

  pager->StorePage<Freelist, FreelistBuilder>(*freelist, page_location);
  auto retrieved = pager->RetrievePage<Freelist, FreelistBuilder>(page_location);

  REQUIRE(retrieved->Previous() == freelist->Previous());
  REQUIRE(retrieved->Next() == freelist->Next());
  REQUIRE(retrieved->Size() == freelist->Size());
  REQUIRE(retrieved->FreePageAt(0) == freelist->FreePageAt(0));
  REQUIRE(retrieved->FreePageAt(1) == freelist->FreePageAt(1));
}


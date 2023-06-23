/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include "backend/BPlusTree.h"
#include "backend/Pager.h"
#include "backend/Types.h"
#include "backend/vfs/unix/UnixVFS.h"
#include "backend/concurrent/unix/UnixFileLock.h"
#include "backend/concurrent/unix/UnixSharedFileLock.h"

using namespace noid::backend;
using namespace noid::backend::vfs;

using UnixPager = Pager<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>>;
using UnixTree = BPlusTree<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>>;

TEST_CASE("Create a new BPlusTree in a temporary file")
{
  auto vfs = std::make_unique<UnixVFS>();
  auto file =  vfs->CreateTempFile();
  std::shared_ptr<UnixPager> pager = UnixPager::Open(std::move(file));
  auto tree = UnixTree::Create(pager, page::TreeType::Table);

  SearchKey key = {1, 3, 3, 7};
  V value = {1, 3, 3, 8};
  REQUIRE(tree->Insert(key, std::forward<V>(value)) == InsertType::Insert);

  auto removed = tree->Remove(key);

  // TODO
  //REQUIRE(removed.has_value());

  //REQUIRE(std::unwrap_reference_t<V>(*tree->Remove(key)) == value);
}
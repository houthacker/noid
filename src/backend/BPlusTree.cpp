/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "BPlusTree.h"

namespace noid::backend {

template<Lockable Lockable, SharedLockable SharedLockable>
BPlusTree<Lockable, SharedLockable>::BPlusTree(std::unique_ptr<Pager<Lockable, SharedLockable>> pager)
    :pager(std::move(pager)) { }

template<Lockable Lockable, SharedLockable SharedLockable>
InsertType BPlusTree<Lockable, SharedLockable>::Insert(const SearchKey& key, V& value)
{
  return InsertType::Insert;
}

template<Lockable Lockable, SharedLockable SharedLockable>
std::optional<V> BPlusTree<Lockable, SharedLockable>::Remove(const SearchKey& key)
{
  return std::nullopt;
}

}
/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_BPLUSTREE_H_
#define NOID_SRC_BACKEND_BPLUSTREE_H_

#include <memory>
#include <optional>

#include "backend/concurrent/Concepts.h"
#include "Pager.h"
#include "Types.h"

namespace noid::backend {

template<Lockable Lockable, SharedLockable SharedLockable>
class BPlusTree {
 private:
    std::unique_ptr<Pager<Lockable, SharedLockable>> pager;

 public:

    /**
     * @brief Creates a new @c BPlusTree using the given @c Pager to read and write tree pages to storage.
     *
     * @param pager The pager to use.
     */
    explicit BPlusTree(std::unique_ptr<Pager<Lockable, SharedLockable>> pager);

    /**
     * @brief Inserts the given key/value pair into this tree, overwriting any pre-existing value having
     * the same key.
     *
     * @param key The key to later retrieve the value with.
     * @param value The actual data to be stored.
     * @return The type of insert.
     */
    InsertType Insert(const SearchKey & key, V& value);

    /**
     * @brief Removes the given value from the tree and returns its associated value.
     *
     * @param key The key to remove.
     * @return The associated value, or an empty optional if no such record exists.
     */
    std::optional<V> Remove(const SearchKey& key);
};

}

#endif //NOID_SRC_BACKEND_BPLUSTREE_H_

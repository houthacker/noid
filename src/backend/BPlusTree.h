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

/**
 * @brief B+Tree implementation that allows for different types of storage.
 * @details Every connection gets its own @c BPlusTree instance during query execution.
 * The @c BPlusTree::header_location is never updated, thus ensuring the same database snapshot will be used throughout,
 * regardless of query run time.
 *
 * @tparam Lockable The Lockable used by the @c Pager for unique locking.
 * @tparam SharedLockable The Lockable used by the @c Pager for shared locking.
 *
 * @see https://en.cppreference.com/w/cpp/named_req/Lockable
 * @see https://en.cppreference.com/w/cpp/named_req/SharedLockable
 * @author houthacker
 */
template<Lockable Lockable, SharedLockable SharedLockable>
class BPlusTree {
 private:

    /**
     * @brief The @c Pager used to access the b+tree pages.
     */
    std::shared_ptr<Pager<Lockable, SharedLockable>> pager;

    /**
     * @brief The location of the @c TreeHeader page of this @c BPlusTree in (persistent) storage.
     */
    PageNumber header_location;

    /**
     * @brief Creates a new @c BPlusTree using the given @c Pager to read and write tree pages to storage.
     *
     * @param pager The pager to use.
     * @param header The number of the header page for this @c BPlusTree.
     */
    BPlusTree(std::shared_ptr<Pager<Lockable, SharedLockable>> pager, PageNumber header)
        :pager(std::move(pager)), header_location(header) { }

 public:

    /**
     * @brief Creates a new @c BPlusTree instance using the given pager and header location.
     *
     * @param pager The @c Pager to use.
     * @param header The location of the associated tree header page.
     * @return A managed pointer to the new @c BPlusTree instance.
     */
    static std::unique_ptr<BPlusTree<Lockable, SharedLockable>> Create(
        std::shared_ptr<Pager<Lockable, SharedLockable>> pager, PageNumber header)
    {
      return std::unique_ptr(new BPlusTree(std::move(pager), header));
    }

    /**
     * @brief Inserts the given key/value pair into this tree, overwriting any pre-existing value having
     * the same key.
     *
     * @param key The key to later retrieve the value with.
     * @param value The actual data to be stored.
     * @return The type of insert.
     */
    InsertType Insert(const SearchKey& key, V& value)
    {
      return InsertType::Insert;
    }

    /**
     * @brief Removes the given value from the tree and returns its associated value.
     *
     * @param key The key to remove.
     * @return The associated value, or an empty optional if no such record exists.
     */
    std::optional<V> Remove(const SearchKey& key)
    {
      return std::nullopt;
    }
};

}

#endif //NOID_SRC_BACKEND_BPLUSTREE_H_

/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_BPLUSTREE_H_
#define NOID_SRC_BACKEND_BPLUSTREE_H_

#include <functional>
#include <memory>
#include <optional>

#include "Bits.h"

#define NOID_INTERNAL_API
#include "BPlusTreeHelper.h"
#undef NOID_INTERNAL_API

#include "Pager.h"
#include "Types.h"
#include "backend/concurrent/Concepts.h"
#include "backend/page/TreeHeader.h"
#include "backend/page/InternalNode.h"
#include "backend/page/LeafNode.h"
#include "backend/page/Overflow.h"

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
    std::unique_ptr<const page::TreeHeader> header;

    /**
     * @brief Creates a @c BPlusTree using the given @c Pager and reads the header page from storage.
     * @details This constructor assumes the tree and @p header_page already exist in storage.
     * If a new tree must be created, use <code>BPlusTree(Pager, PageNumber, TreeType)</code>.
     *
     * @param pager The pager to use.
     * @param header The number of the header page for this @c BPlusTree.
     */
    BPlusTree(std::shared_ptr<Pager<Lockable, SharedLockable>> pager, PageNumber header_page)
        :pager(std::move(pager))
    {
      this->header = this->pager->template ReadPage<page::TreeHeader, page::TreeHeaderBuilder>(header_page);
    }

    /**
     * @brief Creates a new @c BPlusTree of the given type.
     * @details This constructor assumes this is a completely new tree of the given type. Therefore it creates
     * a new @c TreeHeader and instructs @p pager to store it.
     *
     * @param pager The pager to use.
     * @param type The tree type.
     */
    BPlusTree(std::shared_ptr<Pager<Lockable, SharedLockable>> pager, page::TreeType type)
        :pager(pager), header(
        this->pager->template NewBuilder<page::TreeHeader, page::TreeHeaderBuilder>()->WithTreeType(type)->Build())
    {
      this->pager->template WritePage<page::TreeHeader, page::TreeHeaderBuilder>(*this->header);
    }

 public:

    /**
     * @brief Opens an existing @c BPlusTree instance using the given pager and header location.
     *
     * @param pager The @c Pager to use.
     * @param header The location of the associated tree header page.
     * @return A managed pointer to the @c BPlusTree instance.
     */
    static std::unique_ptr<BPlusTree<Lockable, SharedLockable>> Open(
        std::shared_ptr<Pager<Lockable, SharedLockable>> pager, PageNumber header)
    {
      return std::unique_ptr(new BPlusTree<Lockable, SharedLockable>(std::move(pager), header));
    }

    /**
     * @brief Creates and stores a new @c BPlusTree of type @p type using the given @p pager.
     *
     * @param pager The @c Pager to use.
     * @param type The type of tree to create.
     * @return A managed pointer to the @c BPlusTree instance.
     */
    static std::unique_ptr<BPlusTree<Lockable, SharedLockable>> Create(
        std::shared_ptr<Pager<Lockable, SharedLockable>> pager, page::TreeType type)
    {
      return std::unique_ptr<BPlusTree<Lockable, SharedLockable>>(
          new BPlusTree<Lockable, SharedLockable>(std::move(pager), type));
    }

    /**
     * @brief Inserts the given key/value pair into this tree, overwriting any pre-existing value having
     * the same key.
     *
     * @param key The key to later retrieve the value with.
     * @param value The actual data to be stored.
     * @return The type of insert.
     */
    InsertType Insert(SearchKey& key, const V& value)
    {

      // Store any overflow pages first, so we don't have to insert nodes if this fails.
      auto page_range = this->pager->ClaimNextPageRange(this->pager->CalculateOverflow(value));
      details::WriteOverflow(value, page_range, this->pager);

      const auto self = this;
      auto type = InsertType::Insert;

      if (self->header->GetRoot() == NULL_PAGE) {
        auto root_node = this->pager->template NewBuilder<page::LeafNode, page::LeafNodeBuilder>()
            ->WithRecord(details::CreateNodeRecordBuilder(key, value, page_range.first)->Build())
            ->Build();
        auto root_page = this->pager->template WritePage<page::LeafNode, page::LeafNodeBuilder>(*root_node);

        auto new_header = page::TreeHeader::NewBuilder(*this->header)
            ->WithRootPageNumber(root_page)
            ->IncrementPageCount(1)
            ->Build();
        this->pager->template WritePage<page::TreeHeader, page::TreeHeaderBuilder>(*new_header);
        this->header = std::move(new_header); //todo add old header to freelist

        return type;
      }

      return type;
    }

    /**
     * @brief Searches for the given key and returns a reference to the related value if it resides
     * in this @c BPlusTree.
     *
     * @param key The search key.
     * @return The associated value, or an empty optional if no such record exists.
     */
    std::optional<std::reference_wrapper<V>> Search(SearchKey& key)
    {
      return std::nullopt;
    }

    /**
     * @brief Removes the given value from the tree and returns its associated value.
     *
     * @param key The key to remove.
     * @return The associated value, or an empty optional if no such record exists.
     */
    std::optional<V> Remove(SearchKey& key)
    {
      return std::nullopt;
    }
};

}

#endif //NOID_SRC_BACKEND_BPLUSTREE_H_

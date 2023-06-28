/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_INTERNALNODE_H_
#define NOID_SRC_BACKEND_PAGE_INTERNALNODE_H_

#include <array>
#include <memory>
#include <vector>

#include "backend/DynamicArray.h"
#include "backend/Types.h"
#include "Node.h"

namespace noid::backend::page {

class InternalNodeBuilder;

struct NodeEntry {

    /**
     * @brief The entry key.
     */
    SearchKey key;

    /**
     * @brief The number of the right child page.
     */
    PageNumber right_child;

    /**
     * @return The search key of this entry.
     */
    [[nodiscard]] const SearchKey& GetKey() const;

    /**
     * @brief Compares two @c NodeEntry instances for equality.
     * @details Two @c NodeEntry instances are considered equal if all fields have an equal value.
     *
     * @param other The @c NodeEntry to compare to.
     * @return Whether @p other is considered equal to this @c NodeEntry.
     */
    bool operator==(const NodeEntry& other) const;
};

class InternalNode : public Node {
 public:
    static const uint8_t HEADER_SIZE = 24;

 private:
    friend class InternalNodeBuilder;

    /**
     * @brief The location of this page within the database file.
     */
    const PageNumber location;

    /**
     * @brief The position of the next empty entry slot in this @c InternalNode.
     */
    const uint16_t next_entry_slot;

    /**
     * @brief The page number of the leftmost child page.
     * @details A @c NodeEntry stores only the child page to its right. This leaves the child page at index 0
     * unreachable, and therefore the leftmost child node is referred to separately using this field. </br>
     * The penalty of storing child pages like this is that this page is stored outside the vector and therefore is
     * not stored contiguously in memory. But since a @c PageNumber is itself a reference to a page this does not
     * matter because child pages are not stored contiguously anyway.
     */
    const PageNumber leftmost_child;

    /**
     * @brief The list of @c NodeEntry instances.
     */
    const DynamicArray<NodeEntry> entries;

    /**
     * The size in bytes of @c InternalNode page. The page size is required when serializing a @c InternalNode instance,
     * but is itself not serialized into the @c InternalNode. Instead, the page size is stored in the serialized
     * @c FileHeader of a database.
     */
    const uint16_t page_size;

    InternalNode(PageNumber location, uint16_t next_entry_slot, PageNumber leftmost_child, DynamicArray<NodeEntry>&& entries,
        uint16_t page_size);

 public:

    ~InternalNode() override = default;

    /**
     * @brief Creates a new builder for @c InternalNode instances.
     *
     * @param page_size The page size in bytes.
     * @return The new builder instance.
     * @throws std::length_error if @p page_size is too small to contain a @c InternalNode.
     */
    static std::shared_ptr<InternalNodeBuilder> NewBuilder(uint16_t page_size);

    /**
     * @brief Creates a new builder for @c InternalNode instances, using @c base as a starting point.
     *
     * @param base The @c InternalNode to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::shared_ptr<InternalNodeBuilder> NewBuilder(const InternalNode& base);

    /**
     * @brief Creates a new builder for @c InternalNode instances, using @c base as a starting point.
     *
     * @param base The raw bytes to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw bytes are not a valid @c InternalNode.
     */
    static std::shared_ptr<InternalNodeBuilder> NewBuilder(DynamicArray<byte>&& base);

    /**
     * @return The location within the database file.
     */
    [[nodiscard]] PageNumber GetLocation() const;

    /**
     * @return The amount of entries contained in this @c InternalNode
     */
    [[nodiscard]] uint16_t Size() const;

    /**
     * @return The page number of the leftmost child node. A @c PageNumber of 0 indicates there is no such child.
     */
    [[nodiscard]] PageNumber GetLeftmostChild() const;

    /**
     * @param slot The position at which the entry resides.
     * @return A reference to the @c NodeEntry at the given position.
     * @throws std::out_of_range if @c slot is outside of the entry range.
     */
    [[nodiscard]] const NodeEntry& EntryAt(uint16_t slot) const;

    /**
     * @param key The search key.
     * @return Whether this node contains the given key.
     */
    [[nodiscard]] bool Contains(const SearchKey& key) const override;

    /**
     * @brief Serializes this instance to a dynamic byte array.
     *
     * @return The serialized @c InternalNode.
     */
    [[nodiscard]] DynamicArray<byte> ToBytes() const;
};

 class InternalNodeBuilder : public std::enable_shared_from_this<InternalNodeBuilder> {
 private:
    friend class InternalNode;

    /**
     * @brief The location of the page withing the database file.
     */
    PageNumber location = NULL_PAGE;

    /**
     * @brief The configured page size in bytes. Defaults to @c NoidConfig::vfs_page_size.
     */
    uint16_t page_size;

    /**
     * @brief The maximum @c NodeEntry slot, based on the page size.
     */
    uint16_t max_entry_slot;

    /**
     * The number of the leftmost child page.
     */
    PageNumber leftmost_child;

    /**
     * @brief The raw entries in serialized format.
     */
    std::vector<NodeEntry> entries;

    explicit InternalNodeBuilder(uint16_t page_size);
    explicit InternalNodeBuilder(const InternalNode& base);
    explicit InternalNodeBuilder(DynamicArray<byte>&& base);

 public:

    InternalNodeBuilder() = delete;

    /**
     * @brief Creates a new @c InternalNode based on the provided data.
     *
     * @return The new @c InternalNode instance.
     */
    [[nodiscard]] std::unique_ptr<const InternalNode> Build();

    /**
     * @return Whether the node-to-be contains the maximum amount of entries.
     */
    bool IsFull() const;

     /**
      * @brief Sets or overwrites the location of the InternalNode within the database file.
      *
      * @param loc The absolute location within the database file.
      * @return A reference to this builder to support a fluent interface.
      * @throws std::domain_error if @p loc is @c NULL_PAGE.
      */
     std::shared_ptr<InternalNodeBuilder> WithLocation(PageNumber loc);

    /**
     * @brief Sets the leftmost child page for the @c InternalNode
     *
     * @param page_number The page number of the child page.
     * @return A reference to this builder to support a fluent interface.
     */
    std::shared_ptr<InternalNodeBuilder> WithLeftmostChild(PageNumber page_number);

    /**
     * @brief Adds a new entry for the @c InternalNode
     *
     * @param key The entry key.
     * @param right_child_page The child page containing keys larger than or equal to this key.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the node is full (i.e. contains the maximum amount of entries).
     */
    std::shared_ptr<InternalNodeBuilder> WithEntry(SearchKey key, PageNumber right_child_page);

    /**
     * @brief Adds a new entry for the @c InternalNode, possibly overwriting a previously existing entry in
     * that @p slot_hint.
     * @details if @p slot_hint points to a slot not yet occupied, the hint is ignored and the entry is stored
     * in the next free slot.
     *
     * @param key The entry key.
     * @param right_child_page The child page containing keys larger than or equal to this key.
     * @param slot_hint The desired slot at which to insert the entry.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the node is full (i.e. contains the maximum amount of entries).
     */
    std::shared_ptr<InternalNodeBuilder> WithEntry(SearchKey key, PageNumber right_child_page, uint16_t slot_hint);
};

}

#endif //NOID_SRC_BACKEND_PAGE_INTERNALNODE_H_

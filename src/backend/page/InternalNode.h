/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_INTERNALNODE_H_
#define NOID_SRC_BACKEND_PAGE_INTERNALNODE_H_

#include <array>
#include <memory>
#include <vector>
#include "backend/Types.h"

namespace noid::backend::page {

class InternalNodeBuilder;

struct NodeEntry {

    /**
     * @brief The entry key.
     */
    std::array<byte, FIXED_KEY_SIZE> key;

    /**
     * @brief The number of the right child page.
     */
    PageNumber right_child;

    /**
     * @brief Compares two @c NodeEntry instances for equality.
     * @details Two @c NodeEntry instances are considered equal if all fields have an equal value.
     *
     * @param other The @c NodeEntry to compare to.
     * @return Whether @p other is considered equal to this @c NodeEntry.
     */
    bool operator==(const NodeEntry& other) const;
};

class InternalNode {
 private:
    friend class InternalNodeBuilder;

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
    const std::vector<NodeEntry> entries;

    explicit InternalNode(PageNumber leftmost_child, std::vector<NodeEntry>&& entries);

 public:

    /**
     * @brief Creates a new builder for @c InternalNode instances, using an all-zeroes backing vector.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<InternalNodeBuilder> NewBuilder();

    /**
     * @brief Creates a new builder for @c InternalNode instances, using @c base as a starting point.
     *
     * @param base The @c InternalNode to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<InternalNodeBuilder> NewBuilder(const InternalNode& base);

    /**
     * @brief Creates a new builder for @c InternalNode instances, using @c base as a starting point.
     *
     * @param base The raw entries to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw entries is not a valid @c InternalNode.
     */
    static std::unique_ptr<InternalNodeBuilder> NewBuilder(std::vector<byte>&& base);

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
};

class InternalNodeBuilder {
 private:
    friend class InternalNode;

    /**
     * @brief The configured page size in bytes. Defaults to @c NoidConfig::vfs_page_size.
     */
    uint16_t page_size;

    /**
     * @brief The maximum @c NodeEntry slot, based on the page size.
     */
    uint16_t max_slot;

    /**
     * The number of the leftmost child page.
     */
    PageNumber leftmost_child;

    /**
     * @brief The raw entries in serialized format.
     */
    std::vector<NodeEntry> entries;

    explicit InternalNodeBuilder();
    explicit InternalNodeBuilder(const InternalNode& base);
    explicit InternalNodeBuilder(std::vector<byte>&& base);

 public:

    /**
     * @brief Creates a new @c InternalNode based on the provided data.
     *
     * @return The new @c InternalNode instance.
     */
    [[nodiscard]] std::unique_ptr<const InternalNode> Build();

    /**
     * @return Whether the node-to-be contains the maximum amount of entries.
     */
    bool IsFull();

    /**
     * @brief Sets the leftmost child page for the @c InternalNode
     *
     * @param page_number The page number of the child page.
     * @return A reference to this builder to support a fluent interface.
     */
    InternalNodeBuilder& WithLeftmostChild(PageNumber page_number);

    /**
     * @brief Adds a new entry for the @c InternalNode
     *
     * @param key The entry key.
     * @param right_child_page The child page containing keys larger than or equal to this key.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the node is full (i.e. contains the maximum amount of entries).
     */
    InternalNodeBuilder& WithEntry(std::array<byte, FIXED_KEY_SIZE> key, PageNumber right_child_page);

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
    InternalNodeBuilder& WithEntry(std::array<byte, FIXED_KEY_SIZE> key, PageNumber right_child_page,
        uint16_t slot_hint);
};

}

#endif //NOID_SRC_BACKEND_PAGE_INTERNALNODE_H_

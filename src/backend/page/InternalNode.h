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
  std::array<byte, FIXED_KEY_SIZE> key;
  PageNumber right_child;
};

class InternalNode {
 private:
    friend class InternalNodeBuilder;

    /**
     * @brief The raw data in serialized format.
     */
    std::vector<byte> const data;

    explicit InternalNode(std::vector<byte> && data);

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
    static std::unique_ptr<InternalNodeBuilder> NewBuilder(const InternalNode &base);

    /**
     * @brief Creates a new builder for @c InternalNode instances, using @c base as a starting point.
     *
     * @param base The raw data to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw data is not a valid @c InternalNode.
     */
    static std::unique_ptr<InternalNodeBuilder> NewBuilder(std::vector<byte> &&base);

    /**
     * @return The amount of entries contained in this @c InternalNode
     */
    [[nodiscard]] uint8_t GetEntryCount() const;

    /**
     * @return The page number of the leftmost child node. A @c PageNumber of 0 indicates there is no such child.
     */
    [[nodiscard]] PageNumber GetLeftmostChild() const;

    /**
     * @param pos The position at which the entry resides.
     * @return The @c NodeEntry at the given position.
     * @throws std::out_of_range if @c pos is outside of the entry range.
     */
    [[nodiscard]] NodeEntry EntryAt(uint8_t pos) const;
};

class InternalNodeBuilder {
 private:
    friend class InternalNode;

    /**
     * @brief The configured page size in bytes. Defaults to @c NoidConfig::vfs_page_size.
     */
    uint16_t page_size;

    /**
     * @brief The raw data in serialized format.
     */
    std::vector<byte> data;

    explicit InternalNodeBuilder();
    explicit InternalNodeBuilder(const InternalNode &base);
    explicit InternalNodeBuilder(std::vector<byte> &&base);

 public:

    explicit InternalNodeBuilder(InternalNode &&) =delete;

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
    InternalNodeBuilder& WithLeftmostChildPage(PageNumber page_number);

    /**
     * @brief Adds a new entry for the @c InternalNode
     * @param key The entry key.
     * @param right_child_page The child page containing keys larger than or equal to this key.
     * @return std::overflow_error if the node is full (i.e. contains the maximum amount of entries).
     */
    InternalNodeBuilder& WithEntry(std::array<byte, FIXED_KEY_SIZE> key, PageNumber right_child_page);
};

}

#endif //NOID_SRC_BACKEND_PAGE_INTERNALNODE_H_

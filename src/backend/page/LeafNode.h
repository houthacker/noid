/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_LEAFNODE_H
#define NOID_LEAFNODE_H

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "backend/DynamicArray.h"
#include "backend/Types.h"
#include "Node.h"
#include "NodeRecord.h"

namespace noid::backend::page {

class LeafNodeBuilder;

/**
 * @brief Node type to contain the actual data.
 */
class LeafNode : public Node {
 public:

    static const uint8_t HEADER_SIZE = 24;

 private:
    friend class LeafNodeBuilder;

    /**
     * @brief The location of this page withing the database file.
     */
    const PageNumber  location;

    /**
     * @brief The position of the next empty record slot in this @c LeafNode.
     */
    const uint16_t next_record_slot;

    /**
     * @brief The number of the left sibling @c LeafNode page.
     */
    const PageNumber left_sibling;

    /**
     * @brief The number of the right sibling @c LeafNode page.
     */
    const PageNumber right_sibling;

    /**
     * @brief The records containing the actual data.
     */
    const DynamicArray<NodeRecord> records;

    /**
     * The size in bytes of @c TreeHeader page. The page size is required when serializing a @c TreeHeader instance,
     * but is itself not serialized into the @c TreeHeader. Instead, the page size is stored in the serialized
     * @c FileHeader of a database.
     */
    const uint16_t page_size;

    LeafNode(PageNumber location, uint16_t next_record_slot, PageNumber left_sibling, PageNumber right_sibling, DynamicArray<NodeRecord> && records, uint16_t page_size);
 public:

    ~LeafNode() override =default;

    /**
     * @brief Creates a new builder for @c LeafNode instances with all fields unset.
     *
     * @param page_size The page size in bytes.
     * @return The new builder instance.
     * @throws std::length_error if @p page_size is too small to contain a @c LeafNode.
     */
    static std::shared_ptr<LeafNodeBuilder> NewBuilder(uint16_t page_size);

    /**
     * @brief Creates a new builder for @c LeafNode instances, using @c base as a starting point.
     *
     * @param base The @c LeafNode to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::shared_ptr<LeafNodeBuilder> NewBuilder(const LeafNode& base);

    /**
     * @brief Creates a new builder for @c LeafNode instances, using @c base as a starting point.
     *
     * @param base The raw bytes to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw bytes do not represent a valid @c LeafNode.
     */
    static std::shared_ptr<LeafNodeBuilder> NewBuilder(DynamicArray<byte> && base);

    /**
     * @return The location within the database file.
     */
    [[nodiscard]] PageNumber GetLocation() const;

    /**
     * @return The amount of records in this @c LeafNode.
     */
    [[nodiscard]] uint16_t Size() const;

    /**
     * @return The page number of the left sibling node.
     */
    [[nodiscard]] PageNumber GetLeftSibling() const;

    /**
     * @return The page number of the right sibling node.
     */
    [[nodiscard]] PageNumber GetRightSibling() const;

    /**
     *
     * @param slot The slot at which the record resides.
     * @return A reference to the @c NodeRecord at the given position.
     * @throws std::out_of_range if @c slot is outside of the record range.
     */
    [[nodiscard]] const NodeRecord& RecordAt(uint16_t slot) const;

    /**
     * @param key The search key.
     * @return Whether this node contains the given key.
     */
    [[nodiscard]] bool Contains(const SearchKey& key) const override;

    /**
     * @brief Creates a new @c DynamicArray and serializes this @c LeafNode into it.
     *
     * @return The serialized @c Overflow instance.
     */
    [[nodiscard]] DynamicArray<byte> ToBytes() const;
};

 class LeafNodeBuilder : public std::enable_shared_from_this<LeafNodeBuilder> {
 private:
    friend class LeafNode;

     /**
      * @brief The location of the page withing the database file.
      */
     PageNumber location = NULL_PAGE;

    /**
     * @brief The configured page size in bytes. Defaults to @c NoidConfig::vfs_page_size.
     */
    uint16_t page_size;

    /**
     * @brief The maximum @c NodeRecord slot, based on the page size.
     */
    uint16_t max_record_slot;

    /**
     * @brief The left sibling page, or @c 0 if no such page exists.
     */
    PageNumber left_sibling;

    /**
     * @brief The right sibling page, or @c 0 if no such page exists.
     */
    PageNumber right_sibling;

    /**
     * @brief The node records.
     */
    std::vector<NodeRecord> records;

    explicit LeafNodeBuilder(uint16_t page_size);
    explicit LeafNodeBuilder(const LeafNode& base);
    explicit LeafNodeBuilder(DynamicArray<byte> && base);

 public:

    LeafNodeBuilder() =delete;

    /**
     * @brief Creates a new @c LeafNode based on the provided data.
     *
     * @return The new @c LeafNode instance.
     */
    [[nodiscard]] std::unique_ptr<const LeafNode> Build();

    /**
     * @return Whether the maximum amount of slots are occupied.
     */
    bool IsFull() const;

     /**
       * @brief Sets or overwrites the location of the LeafNode within the database file.
       *
       * @param loc The absolute location within the database file.
       * @return A reference to this builder to support a fluent interface.
       * @throws std::domain_error if @p loc is @c NULL_PAGE.
       */
     std::shared_ptr<LeafNodeBuilder> WithLocation(PageNumber loc);

    /**
     * @brief Sets the left sibling for the @c LeafNode.
     *
     * @param sibling The page number of the left sibling.
     * @return A reference to this builder to support a fluent interface.
     */
    std::shared_ptr<LeafNodeBuilder> WithLeftSibling(PageNumber sibling);

    /**
     * @brief Sets the right sibling for the @c LeafNode.
     * @param sibling The page number of the left sibling.
     * @return A reference to this builder to support a fluent interface.
     */
    std::shared_ptr<LeafNodeBuilder> WithRightSibling(PageNumber sibling);

    /**
     * @brief Adds the given record to the list of records for the new @c LeafNode.
     *
     * @param record The record to add.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the node is full (i.e. contains the maximum amount of records).
     */
    std::shared_ptr<LeafNodeBuilder> WithRecord(NodeRecord record);

    /**
     * @brief Adds the given record to the list of records for the new @c LeafNode, overwriting any pre-existing
     * @c NodeRecord in the hinted slot.
     * @details if @p slot_hint points to a slot not yet occupied, the hint is ignored and the entry is stored
     * in the next free slot.
     *
     * @param record The record to add.
     * @param slot_hint The desired slot at which to insert the record.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the node is full (i.e. contains the maximum amount of records).
     */
    std::shared_ptr<LeafNodeBuilder> WithRecord(NodeRecord record, uint16_t slot_hint);
};

}

#endif //NOID_LEAFNODE_H

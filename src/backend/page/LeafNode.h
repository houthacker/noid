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

namespace noid::backend::page {

class LeafNodeBuilder;

/**
 * @brief Data object for the node record which contains the actual data.
 * @note Since this is not a page type, it does not get its own translation unit.
 */
class NodeRecord : public Node {
 public:
    static uint8_t const INLINE_PAYLOAD_SIZE = 7;

 private:
    /**
     * @brief The search key that is associated with the payload.
     */
    std::array<byte, FIXED_KEY_SIZE> key;

    /**
     * @brief Indicates if all payload data fits in this record.
     * @details If set to a non-zero value, this value is the amount of payload bytes. Otherwise,
     * the payload does not fit in this record and will have at least one @c Overflow to contain
     * the data.
     */
    byte inline_indicator;

    /**
     * @brief The payload data.
     * @details If @c NodeRecord::inline_indicator is set to non-zero, that value is the payload
     * data size. Otherwise, the first three bytes are actual payload and the last four bytes
     * contain the first overflow page number.
     */
    std::array<byte, INLINE_PAYLOAD_SIZE> payload;

 public:

    /**
     * @brief Default constructor.
     */
    NodeRecord() = default;

    /**
     * @brief Creates a new @c NodeRecord with the given data.
     * @details The payload is either all payload, or in the case the payload overflows, the last four bytes
     * contain the page number of the first overflow page.
     *
     * @param key The record key.
     * @param inline_indicator The size of the inline payload, or zero if the payload also resides in overflow pages.
     * @param payload The payload.
     */
    NodeRecord(std::array<byte, FIXED_KEY_SIZE> key, byte inline_indicator, std::array<byte, INLINE_PAYLOAD_SIZE> payload);

    /**
     * @return The record key.
     */
    [[nodiscard]] const std::array<byte, FIXED_KEY_SIZE>& GetKey() const;

    /**
     * @details Returns the byte size of the payload if the full payload fits in this record. If at least
     * one overflow page is necessary, this value is 0 and the payload is divided in 3 bytes payload and 4 bytes
     * containing the page number of the first overflow page.
     *
     * @return The inline payload size, or zero if the payload overflows.
     */
    [[nodiscard]] byte GetInlineIndicator() const;

    /**
     * @return The payload bytes.
     */
    [[nodiscard]] const std::array<byte, INLINE_PAYLOAD_SIZE>& GetPayload() const;

    /**
     * @brief Interprets the last four bytes of the payload as a @c PageNumber. If @c NodeRecord::inline_indicator
     * is set to non-zero, calling this method will throw an exception since the retrieved value
     * is not a @c PageNumber.
     *
     * @return The first overflow page number.
     * @throws std::domain_error if called when @c NodeRecord::inline_indicator is non-zero.
     */
    [[nodiscard]] PageNumber GetOverflowPage() const;

    bool operator==(const NodeRecord& other) const;
};

/**
 * @brief Node type to contain the actual data.
 */
class LeafNode {
 private:
    friend class LeafNodeBuilder;

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

    LeafNode(PageNumber left_sibling, PageNumber right_sibling, DynamicArray<NodeRecord> && records);
 public:

    /**
     * @brief Creates a new builder for @c LeafNode instances with all fields unset.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<LeafNodeBuilder> NewBuilder();

    /**
     * @brief Creates a new builder for @c LeafNode instances, using @c base as a starting point.
     *
     * @param base The @c LeafNode to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<LeafNodeBuilder> NewBuilder(const LeafNode& base);

    /**
     * @brief Creates a new builder for @c LeafNode instances, using @c base as a starting point.
     *
     * @param base The raw bytes to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw bytes do not represent a valid @c LeafNode.
     */
    static std::unique_ptr<LeafNodeBuilder> NewBuilder(DynamicArray<byte> && base);

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
};

class LeafNodeBuilder {
 private:
    friend class LeafNode;

    /**
     * @brief The configured page size in bytes. Defaults to @c NoidConfig::vfs_page_size.
     */
    uint16_t page_size;

    /**
     * @brief The maximum @c NodeRecord slot, based on the page size.
     */
    uint16_t max_slot;

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

    explicit LeafNodeBuilder();
    explicit LeafNodeBuilder(const LeafNode& base);
    explicit LeafNodeBuilder(DynamicArray<byte> && base);

 public:

    /**
     * @brief Creates a new @c LeafNode based on the provided data.
     *
     * @return The new @c LeafNode instance.
     */
    [[nodiscard]] std::unique_ptr<const LeafNode> Build();

    /**
     * @return Whether the maximum amount of slots are occupied.
     */
    bool IsFull();

    /**
     * @brief Sets the left sibling for the @c LeafNode.
     *
     * @param sibling The page number of the left sibling.
     * @return A reference to this builder to support a fluent interface.
     */
    LeafNodeBuilder& WithLeftSibling(PageNumber sibling);

    /**
     * @brief Sets the right sibling for the @c LeafNode.
     * @param sibling The page number of the left sibling.
     * @return A reference to this builder to support a fluent interface.
     */
    LeafNodeBuilder& WithRightSibling(PageNumber sibling);

    /**
     * @brief Adds the given record to the list of records for the new @c LeafNode.
     *
     * @param record The record to add.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the node is full (i.e. contains the maximum amount of records).
     */
    LeafNodeBuilder& WithRecord(NodeRecord record);

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
    LeafNodeBuilder& WithRecord(NodeRecord record, uint16_t slot_hint);
};

}

#endif //NOID_LEAFNODE_H

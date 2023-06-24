/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_NODERECORD_H
#define NOID_NODERECORD_H

#include <cstdint>
#include <memory>

#include "backend/DynamicArray.h"
#include "backend/Types.h"

namespace noid::backend::page {

class NodeRecordBuilder;

/**
 * @brief Data object for the node record which contains the actual data.
 * @note According to the @c Pager, this is <em>not</em> a Page. But since there is a little complexity involved when
 * creating a @c NodeRecord and possible @c Overflow pages from raw bytes, it implements the Builder pattern to
 * ease that pain.
 *
 * @author houthacker
 */
class NodeRecord {
 public:
    static uint8_t const INLINE_PAYLOAD_SIZE = 7;
    static uint8_t const OVERFLOW_PAYLOAD_SIZE = INLINE_PAYLOAD_SIZE - sizeof(PageNumber);
    static uint8_t const SIZE = sizeof(SearchKey) + sizeof(uint8_t) + INLINE_PAYLOAD_SIZE;

 private:
    friend class NodeRecordBuilder;

    /**
     * @brief The search key that is associated with the payload.
     */
    SearchKey key = {0};

    /**
     * @brief Indicates whether all payload data fits in this record.
     *
     * @details If set to a non-zero value, this value is the amount of inline payload bytes. Otherwise,
     * the payload does not fit in this record and will have at least one @c Overflow to contain
     * the data.
     */
    uint8_t inline_indicator = 0;

    /**
     * @brief The payload data.
     *
     * @details If @c NodeRecord::inline_indicator is set to non-zero, that value is the payload
     * data size. Otherwise, the first three bytes are actual payload and the last four bytes
     * contain the first overflow page number.
     */
    std::array<byte, INLINE_PAYLOAD_SIZE> payload = {0};

    /**
     * @brief Creates a new @c NodeRecord.
     * @details If all payload is inlined, @p payload contains @p inline_indicator bytes data. If the payload overflows,
     * the first 3 bytes of @p payload are payload and the last 4 bytes represent @c uint32_t page number of the
     * first overflow page.
     *
     * @param key The record key.
     * @param inline_indicator The payload size in bytes if all payload is inlined, or zero otherwise.
     * @param payload The payload.
     */
    NodeRecord(SearchKey key, uint8_t inline_indicator, std::array<byte, INLINE_PAYLOAD_SIZE> payload);

 public:

    /**
     * @brief Default constructor.
     */
    NodeRecord() = default;

    /**
     * @brief Creates an empty new builder for @c NodeRecord instances.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<NodeRecordBuilder> NewBuilder();

    /**
     *
     * @param container
     * @param range
     * @return
     */
    static std::unique_ptr<NodeRecordBuilder> NewBuilder(const DynamicArray<byte>& container,
        DynamicArray<byte>::size_type read_idx);

    /**
     * @brief Calculates the amount of @c Overflow pages required to fit @p value.
     *
     * @param value The data to calculate the required amount of overflow pages for.
     * @param page_size The page size in bytes.
     * @return The amount of required @c Overflow pages.
     */
    static uint32_t CalculateOverflow(const V& value, const uint16_t page_size);

    /**
     * @return The record key.
     */
    [[nodiscard]] const SearchKey& GetKey() const;

    /**
     * @details Returns the byte size of the payload if the full payload fits in this record. If at least
     * one overflow page is necessary, this value is 0 and the payload is divided in 3 bytes payload and 4 bytes
     * containing the page number of the first overflow page.
     *
     * @return The inline payload size, or zero if the payload overflows.
     */
    [[nodiscard]] uint8_t GetInlineIndicator() const;

    /**
     * @return The payload bytes.
     */
    [[nodiscard]] const std::array<byte, INLINE_PAYLOAD_SIZE>& GetPayload() const;

    /**
     * @brief Interprets the last four bytes of the payload as a @c PageNumber. If @c NodeRecord::inline_indicator
     * is set to non-zero, calling this method will return @c NULL_PAGE.
     *
     * @return The first overflow page number, or @c NULL_PAGE if no such page exists.
     */
    [[nodiscard]] PageNumber GetOverflowPage() const;

    bool operator==(const NodeRecord& other) const = default;
};

class NodeRecordBuilder {
 private:
    friend class NodeRecord;

    /**
     * @brief The search key that is associated with the payload.
     */
    SearchKey key = {0};

    /**
     * @brief Indicates whether all payload data fits in this record.
     *
     * @details If set to a non-zero value, this value is the amount of inline payload bytes. Otherwise,
     * the payload does not fit in this record and will have at least one @c Overflow to contain
     * the data.
     */
    uint8_t inline_payload_size = 0;

    /**
     * @brief The payload data.
     *
     * @details If @c NodeRecordBuilder::inline_payload_size is set to non-zero, that value is the payload
     * data size. Otherwise, the first three bytes are actual payload and the last four bytes
     * contain the first overflow page number.
     */
    std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE> payload = {0};

    NodeRecordBuilder() = default;

 public:

    /**
     * @brief Sets the search key for the @c NodeRecord.
     *
     * @param search_key The search key.
     * @return A reference to this builder to support a fluent interface.
     */
    NodeRecordBuilder& WithSearchKey(SearchKey search_key);

    /**
     * @brief Sets the inline payload size and writes @p data to the payload.
     *
     * @param data The data to write to the payload.
     * @param data_size The amount of bytes in the payload. All bytes after this are assumed to be garbage.
     * @return A reference to this builder to support a fluent interface.
     */
    NodeRecordBuilder& WithInlinePayload(std::array<byte, NodeRecord::INLINE_PAYLOAD_SIZE> data,
        uint8_t data_size);

    /**
     * @brief Sets the inline indicator to zero and writes the given data and overflow page to the payload.
     *
     * @param data The payload bytes that fit in the @c NodeRecord.
     * @param first_overflow_page The page number of the first overflow page.
     * @return A reference to this builder to support a fluent interface.
     */
    NodeRecordBuilder& WithOverflowPayload(std::array<byte, NodeRecord::OVERFLOW_PAYLOAD_SIZE> data,
        PageNumber first_overflow_page);

    /**
     * @brief Creates a new @c NodeRecord based on the provided values.
     * @details If an empty @c NodeRecordBuilder is built, the returned @c NodeRecord will contain all zero-fields.
     *
     * @return The new @c NodeRecord instance.
     */
    NodeRecord Build() const noexcept;
};

}
#endif //NOID_NODERECORD_H

/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_OVERFLOW_H
#define NOID_OVERFLOW_H

#include <cstdint>
#include <memory>

#include "backend/DynamicArray.h"
#include "backend/Types.h"

namespace noid::backend::page {

class OverflowBuilder;

/**
 * @brief Implementation of the overflow page.
 * @details An @c Overflow page contains data that cannot be inlined within a @c NodeRecord.
 *
 * @see The serialized-format description in the linked documentation at doc/database_file_formats.md
 * @author houthacker
 */
class Overflow {
 public:

    /**
     * @brief The header size in bytes of an @c Overflow page.
     */
    static const uint8_t HEADER_SIZE = 6;

 private:
    friend class OverflowBuilder;

    /**
     * @brief The location of this page withing the database file.
     */
    const PageNumber  location;

    /**
     * @brief The payload size in this page.
     */
    const uint16_t data_size;

    /**
     * @brief The next overflow page, or zero if there is no such page.
     */
    const PageNumber next;

    /**
     * @brief The payload data, including possible zeroed padding.
     * @details Since all pages in a database file are of the same size, @c Overflow pages are aligned
     * to this page size, which can incur padding in the payload data. To read the correct amount of bytes
     * from this page, use @c Overflow::data_size instead of @c Overflow::data.size() to determine
     * the amount of payload bytes.
     */
    const DynamicArray<byte> data;

    /**
     * The size in bytes of @c Overflow page. The page size is required when serializing a @c Overflow instance,
     * but is itself not serialized into the @c Overflow. Instead, the page size is stored in the serialized
     * @c FileHeader of a database.
     */
    const uint16_t page_size;

    Overflow(PageNumber location, uint16_t payload_size, PageNumber next, DynamicArray<byte> && data, uint16_t page_size);

 public:

    ~Overflow() =default;

    /**
     * @brief Creates a new builder for @c Overflow pages with the given page size. All fields are set to zero.
     *
     * @param page_size The page size in bytes.
     * @return The new builder instance.
     * @throws std::length_error if @p page_size is too small to contain a @c Overflow page.
     */
    static std::shared_ptr<OverflowBuilder> NewBuilder(uint16_t page_size);

    /**
     * @brief Creates a new builder for @c Overflow instances, using @c base as a starting point.
     *
     * @param base The @c Overflow to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::shared_ptr<OverflowBuilder> NewBuilder(const Overflow& base);

    /**
     * @brief Creates a new builder for @c Overflow instances, using @c base as a starting point.
     * @note Callers must ensure @c base.size() equals the page size of the originating database.
     *
     * @param base The raw bytes to use as a basis for the new instance.
     * @return The new builder instance.
     * @throw std::invalid_argument if base is empty.
     */
    static std::shared_ptr<OverflowBuilder> NewBuilder(DynamicArray<byte> && base);

    /**
     * @return The location within the database file.
     */
    [[nodiscard]] PageNumber GetLocation() const;

    /**
     * @return The size in bytes of the payload contained in this @c Overflow page.
     */
    [[nodiscard]] uint16_t GetDataSize() const;

    /**
     * @return The location of the next @c Overflow page, or zero if no such page exists.
     */
    [[nodiscard]] PageNumber GetNext() const;

    /**
     * @brief Returns the payload data including possible padding. Use @c Overflow::GetDataSize() to
     * determine the amount of bytes to read from the returned data.
     *
     * @return The payload data.
     */
    [[nodiscard]] const DynamicArray<byte>& GetData() const;

    /**
     * @brief Creates a new @c DynamicArray and serializes this @c Overflow into it.
     *
     * @return The serialized @c Overflow instance.
     */
    [[nodiscard]] DynamicArray<byte> ToBytes() const;
};

 class OverflowBuilder : public std::enable_shared_from_this<OverflowBuilder> {
 private:
    friend class Overflow;

    /**
     * @brief The location of the page within the database file.
     */
    PageNumber location = NULL_PAGE;

    /**
     * @brief The configured page size in bytes. Defaults to @c NoidConfig::vfs_page_size.
     */
    uint16_t page_size;

    /**
     * @brief The size in bytes of the payload contained in the @c Overflow page being built.
     */
    uint16_t data_size = 0;

    /**
     * @brief The next overflow page number, or zero if no such page exists.
     */
    PageNumber next = 0;

    /**
     * @brief The payload data with zeroed padding if so required.
     */
    DynamicArray<byte> data = {0};

    explicit OverflowBuilder(uint16_t page_size);
    explicit OverflowBuilder(const Overflow& base);
    explicit OverflowBuilder(DynamicArray<byte> && base);

 public:

    OverflowBuilder() =delete;

    /**
     * @brief Calculates the maximum data size assuming that @c page_size has been set.
     *
     * @return The maximum data size, or zero if @c page_size is too small to allow for data.
     */
     uint16_t MaxDataSize() const;

    /**
     * @brief Creates a new @c Overflow page based on the provided data.
     *
     * @return The new @c Overflow page instance.
     * @throws std::length_error if @c page_size or @c data_size is zero, or if @c page_size is too small to fit any data.
     * @throws std::domain_error if @c location has not been set.
     */
    [[nodiscard]] std::unique_ptr<const Overflow> Build();

     /**
        * @brief Sets or overwrites the location of the Overflow within the database file.
        *
        * @param loc The absolute location within the database file.
        * @return A reference to this builder to support a fluent interface.
        * @throws std::domain_error if @p loc is @c NULL_PAGE.
        */
     std::shared_ptr<OverflowBuilder> WithLocation(PageNumber loc);

    /**
     * @brief Sets the location of the page_number @c Overflow page.
     *
     * @param page_number The number of the page_number @c Overflow page.
     * @return A reference to this builder to support a fluent interface.
     */
    std::shared_ptr<OverflowBuilder> WithNext(PageNumber page_number);

    /**
     * @brief Shorthand for <code>OverflowBuilder::WithData(bytes, bytes.size())</code>.
     *
     * @param bytes The data bytes.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::length_error if @c bytes.size() is too large to fit in the current @c page_size.
     */
    std::shared_ptr<OverflowBuilder> WithData(DynamicArray<byte> && bytes);

    /**
     * @brief Sets the data of the @c Overflow page.
     *
     * @param bytes The raw bytes.
     * @param size The amount of bytes in the data.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::length_error if @c bytes.size() is too large to fit in the current @c page_size or if @p bytes.size()
     * is less than @p size.
     */
    std::shared_ptr<OverflowBuilder> WithData(DynamicArray<byte> && bytes, uint16_t size);
};

}

#endif //NOID_OVERFLOW_H

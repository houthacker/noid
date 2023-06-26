/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_FREELIST_H_
#define NOID_SRC_BACKEND_PAGE_FREELIST_H_

#include <cstdint>
#include <memory>

#include "backend/Types.h"
#include "backend/DynamicArray.h"

namespace noid::backend::page {

class FreelistBuilder;

/**
 * @brief Freelist implementation.
 * @details The @c Freelist page type stores all database pages that previously were in use, but have become available
 * for reuse. Like all other available page types, a @c Freelist is immutable. Updates to an instance must be made
 * through a @c FreelistBuilder.
 *
 * @see The serialized-format description in the linked documentation at doc/database_file_formats.md
 *
 * @author houthacker
 */
class Freelist {
 public:
    static const uint8_t HEADER_SIZE = 12;

 private:
    friend class FreelistBuilder;

    /**
     * @brief The location of this page within the database file.
     */
    const PageNumber location;

    /**
     * @brief The serialized page size in bytes.
     */
    const uint16_t page_size;

    /**
     * @brief The page number of the previous freelist entry.
     */
    const PageNumber previous;

    /**
     * @brief The page number of the next freelist entry.
     */
    const PageNumber next;

    /**
     * @brief The list of free pages stored in this freelist.
     */
    const DynamicArray<PageNumber> free_pages;

    /**
     * @brief The position of the next free slot in this @c Freelist.
     */
    const uint16_t next_free_slot;

    explicit Freelist(PageNumber location, uint16_t page_size, PageNumber previous, PageNumber next, DynamicArray<PageNumber> free_pages,
        uint16_t next_free_slot);

 public:

    /**
     * @brief Creates a new builder for @c Freelist instances, using an all-zeroes backing vector.
     *
     * @param page_size The page size in bytes.
     * @return The new builder instance.
     * @throws std::length_error if @p page_size is too small to contain the freelist header.
     */
    static std::shared_ptr<FreelistBuilder> NewBuilder(uint16_t page_size);

    /**
     * @brief Creates a new builder for @c Freelist instances, using @c base as a starting point.
     *
     * @param base The @c Freelist to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::shared_ptr<FreelistBuilder> NewBuilder(const Freelist& base);

    /**
     * @brief Creates a new builder for @c Freelist instances, using @c base as a starting point.
     *
     * @param base The raw bytes to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw data do not represent a valid serialized @c Freelist.
     */
    static std::shared_ptr<FreelistBuilder> NewBuilder(DynamicArray<byte>&& base);

    /**
     * @return The location within the database file.
     */
    [[nodiscard]] PageNumber Location() const;

    /**
     * @brief Returns the page number of the previous freelist page. A @c PageNumber of zero means no previous page.
     *
     * @return The previous freelist page number.
     */
    [[nodiscard]] PageNumber Previous() const;

    /**
     * @brief Returns the page number of the next freelist page. A @c PageNumber of zero means no next page.
     *
     * @return The next freelist page number.
     */
    [[nodiscard]] PageNumber Next() const;

    /**
     * @brief The amount of free page numbers stored within this @c Freelist.
     *
     * @return The amount of free page numbers in this @c Freelist.
     */
    [[nodiscard]] uint16_t Size() const;

    /**
     * @brief Returns the page number of the free (reusable) page at position @c pos.
     *
     * @param pos The position within this @c Freelist.
     * @return The page number of the free page.
     * @throws std::out_of_range if @p pos >= @c this->Size().
     */
    [[nodiscard]] PageNumber FreePageAt(uint16_t pos) const;

    /**
     * @brief Creates a new vector and serializes this @c Freelist into it.
     *
     * @return The serialized @c Freelist instance.
     */
    [[nodiscard]] DynamicArray<byte> ToBytes() const;
};

 class FreelistBuilder : public std::enable_shared_from_this<FreelistBuilder> {
 private:

    friend class Freelist;

    /**
     * @brief The location within the database file.
     */
    PageNumber location;

    /**
     * @brief The page number of the previous freelist entry.
     */
    PageNumber previous_freelist_entry;

    /**
     * @brief The page number of the next freelist entry.
     */
    PageNumber next_freelist_entry;

    /**
     * @brief The configured page size in bytes. Defaults to @c NoidConfig::vfs_page_size.
     */
    uint16_t page_size;

    /**
     * @brief The max page position value. Defaults to 1021: (page_size - FREELIST_OFFSET) / sizeof(PageNumber)
     */
    uint16_t max_slot;

    /**
     * @brief The current insert slot.
     */
    uint16_t cursor;

    /**
     * @brief The list of free pages
     */
    std::vector<PageNumber> free_pages;

    explicit FreelistBuilder(uint16_t page_size);
    explicit FreelistBuilder(const Freelist& base);
    explicit FreelistBuilder(DynamicArray<byte>&& base);

 public:

    FreelistBuilder() = delete;

    /**
     * @brief Creates a new @c Freelist based on the provided data.
     *
     * @return The new @c Freelist instance.
     * @throws std::domain_error if @c FreelistBuilder::location is unset.
     */
    [[nodiscard]] std::unique_ptr<const Freelist> Build() const;

    /**
     * @brief Sets or overwrites the location of the Freelist within the database file.
     *
     * @param loc The absolute location within the database file.
     * @return A reference to this builder to support a fluent interface.
     */
    std::shared_ptr<FreelistBuilder> WithLocation(PageNumber loc);

    /**
     * @brief Sets the previous freelist page to the provided value.
     *
     * @param previous The previous freelist page number.
     * @return A reference to this builder to support a fluent interface.
     */
    std::shared_ptr<FreelistBuilder> WithPrevious(PageNumber previous);

    /**
     * @brief Sets the next freelist page number to the provided value.
     *
     * @param next The next freelist page number.
     * @return A reference to this builder to support a fluent interface.
     */
    std::shared_ptr<FreelistBuilder> WithNext(PageNumber next);

    /**
     * @brief Designates the given @c PageNumber as reusable and stores it directly after the
     * current last free page number.
     *
     * @param free_page The page number to designate as reusable.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the @c Freelist is full before adding the given @p free_page
     */
    std::shared_ptr<FreelistBuilder> WithFreePage(PageNumber free_page);

    /**
     * @brief Designates the given @c PageNumber as reusable and stores it at the given index.
     * @details Adds the given @p free_page to the list of reusable page numbers. If @p slot_hint is less than
     * @c next_free_page_position, @p free_page is stored at the given position. Otherwise, it is stored
     * at @c next_free_page_position.
     * This allows to overwrite an existing free page entry, but denies non-contiguous storage of free pages in
     * the list.
     *
     * @param free_page The page number to designate as reusable.
     * @param slot_hint A hint as to where to store the given @p free_page.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the @c Freelist is full before adding the given @p free_page
     */
    std::shared_ptr<FreelistBuilder> WithFreePage(PageNumber free_page, std::size_t slot_hint);
};

}

#endif //NOID_SRC_BACKEND_PAGE_FREELIST_H_

/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_FREELIST_H_
#define NOID_SRC_BACKEND_PAGE_FREELIST_H_

#include <cstdint>
#include <memory>
#include <vector>
#include "backend/Types.h"

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
 private:
    friend class FreelistBuilder;

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
    const std::vector<PageNumber> free_pages;

    explicit Freelist(PageNumber previous, PageNumber next, std::vector<PageNumber> free_pages);

 public:

    /**
     * @brief Creates a new builder for @c Freelist instances, using an all-zeroes backing vector.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<FreelistBuilder> NewBuilder();

    /**
     * @brief Creates a new builder for @c Freelist instances, using @c base as a starting point.
     *
     * @param base The @c Freelist to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<FreelistBuilder> NewBuilder(const Freelist& base);

    /**
     * @brief Creates a new builder for @c Freelist instances, using @c base as a starting point.
     *
     * @param base The raw data to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw data is not a valid serialized @c Freelist.
     */
    static std::unique_ptr<FreelistBuilder> NewBuilder(std::vector<byte> && base);

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
};

class FreelistBuilder {
 private:

    friend class Freelist;

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
    std::size_t max_slot;

    /**
     * @brief The list of free pages
     */
    std::vector<PageNumber> free_pages;

    explicit FreelistBuilder();
    explicit FreelistBuilder(const Freelist & base);
    explicit FreelistBuilder(std::vector<byte> && base);

 public:

    /**
     * @brief Creates a new @c Freelist based on the provided data.
     *
     * @return The new @c Freelist instance.
     */
    [[nodiscard]] std::unique_ptr<const Freelist> Build() const;

    /**
     * @brief Sets the previous freelist page to the provided value.
     *
     * @param previous The previous freelist page number.
     * @return A reference to this builder to support a fluent interface.
     */
    FreelistBuilder& WithPrevious(PageNumber previous);

    /**
     * @brief Sets the next freelist page number to the provided value.
     *
     * @param next The next freelist page number.
     * @return A reference to this builder to support a fluent interface.
     */
    FreelistBuilder& WithNext(PageNumber next);

    /**
     * @brief Designates the given @c PageNumber as reusable and stores it directly after the
     * current last free page number.
     *
     * @param free_page The page number to designate as reusable.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if the @c Freelist is full before adding the given @p free_page
     */
    FreelistBuilder& WithFreePage(PageNumber free_page);

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
    FreelistBuilder& WithFreePage(PageNumber free_page, std::size_t slot_hint);
};

}

#endif //NOID_SRC_BACKEND_PAGE_FREELIST_H_

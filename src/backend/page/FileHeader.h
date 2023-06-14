/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_DATABASEHEADER_H_
#define NOID_SRC_BACKEND_PAGE_DATABASEHEADER_H_

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include "backend/Types.h"

namespace noid::backend::page {

class FileHeaderBuilder;

/**
 * @brief The noid database header format.
 *
 * @author houthacker
 */
class FileHeader {
 public:
    /**
     * @brief The size in bytes of a noid database header on disk.
     */
    static uint8_t const BYTE_SIZE = 100;

 private:
    friend class FileHeaderBuilder;

    /**
     * The byte size of database pages, except for the database header itself, which has a size of @c FileHeader::BYTE_SIZE.
     */
    const uint16_t page_size;

    /**
     * The key size in bytes of all keys in the database.
     */
    const uint8_t key_size;

    /**
     * The number of the first tree header page.
     */
    const PageNumber first_tree_header_page;

    /**
     * The number of the first freelist entry.
     */
    const PageNumber first_freelist_page;

    /**
     * The database header checksum.
     */
    const uint32_t checksum;

    /*
     * Creates a new @c FileHeader
     */
    FileHeader(uint16_t page_size, uint8_t key_size, PageNumber first_tree_header_page, PageNumber first_freelist_page, uint32_t checksum);

 public:

    /**
     * @brief Creates a new builder for @c FileHeader instances, using default values.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<FileHeaderBuilder> NewBuilder();

    /**
     * @brief Creates a new builder for @c FileHeader instances, using @p base as a starting point.
     *
     * @param base The @c FileHeader to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<FileHeaderBuilder> NewBuilder(const FileHeader& base);

    /**
     * @brief Creates a new builder for @c FileHeader instances, using the given raw array as a base.
     * @details The data in this array is expanded to
     *
     * @param base The raw bytes to use as a basis for the new @c FileHeader instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given bytes do not represent a valid @c FileHeader.
     */
    static std::unique_ptr<FileHeaderBuilder> NewBuilder(std::array<byte, FileHeader::BYTE_SIZE> &base);

    /**
     * @brief Creates a new array containing the serialized representation of this database header.
     *
     * @return The serialized database header.
     */
    [[nodiscard]] std::array<byte, FileHeader::BYTE_SIZE> ToBytes() const;

    /**
     * @return The size of the database pages in bytes.
     */
    [[nodiscard]] uint16_t GetPageSize() const;

    /**
     * @return The size of entry keys in bytes.
     */
    [[nodiscard]] uint8_t GetKeySize() const;

    /**
     * @brief Returns the page number of the first tree header page in the database file.
     *
     * @return The page number of the first tree header page.
     */
    [[nodiscard]] PageNumber GetFirstTreeHeaderPage() const;

    /**
     * @brief Returns the page number of the first freelist page in the database file. A value of 0 means
     * there is no freelist.
     *
     * @return The page number of the first freelist page.
     */
    [[nodiscard]] PageNumber GetFirstFreelistPage() const;

    /**
     * @brief Returns the FNV-1a hash of the header data up until the hash itself.
     *
     * @details The header checksum is used to verify the header has not been corrupted.
     * @return The header checksum.
     */
    [[nodiscard]] uint32_t GetChecksum() const;

    /**
     * @details Two instances of @c FileHeader are considered equal if their serialized form contains the same
     * bytes.
     *
     * @param other The instance to compare to.
     * @return Whether this @c FileHeader is considered equal to @p other.
     */
    [[nodiscard]] bool Equals(const FileHeader& other) const;
};

bool operator==(const FileHeader& lhs, const FileHeader& rhs);
bool operator!=(const FileHeader& lhs, const FileHeader& rhs);

/**
 * @brief Builder for @c FileHeader instances.
 *
 * @author houthacker
 */
class FileHeaderBuilder {
 private:
    friend class FileHeader;

    uint16_t page_size;
    uint8_t key_size;
    PageNumber first_tree_header_page;
    PageNumber first_freelist_page;

    explicit FileHeaderBuilder();
    explicit FileHeaderBuilder(std::array<byte, FileHeader::BYTE_SIZE> const & base);

    /**
     * @brief Creates a new builder with default values.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<FileHeaderBuilder> Create();

    /**
     * @brief Creates a new builder with a copy of the the given backing array.
     *
     * @param base The backing array to use as a basis for the new header instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the checksum verification fails.
     */
    static std::unique_ptr<FileHeaderBuilder> Create(std::array<byte, FileHeader::BYTE_SIZE> const & base);

    /**
     * @brief Creates a new builder with a copy of the data in @p base.
     *
     * @param base The header to use as a basis for the new header instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<FileHeaderBuilder> Create(const FileHeader& base);
 public:

    /**
     * @brief Creates a new @c FileHeader instance based on the provided data.
     *
     * @return The new @c FileHeader instance.
     */
    [[nodiscard]] std::unique_ptr<const FileHeader> Build() const;

    /**
     * @brief Sets or overwrites the currently configured page size.
     * @details If @p page_size is less than 512 or not a power of 2, it is set to respectively 512 or
     * the next power of two.
     *
     * @param size The new page size.
     * @return A reference to this builder to support a fluent interface.
     */
    FileHeaderBuilder& WithPageSize(uint16_t size = DEFAULT_PAGE_SIZE);

    /**
     * @brief Sets or overwrites the currently configured key size.
     * @details If @p key_size is less than, or not a multiple of 8, it will be rounded to the
     * next multiple of 8.
     *
     * @param size The new key size.
     * @return A reference to this builder to support a fluent interface.
     */
    FileHeaderBuilder& WithKeySize(uint8_t size = FIXED_KEY_SIZE);

    /**
     * @brief Sets or overwrites the page number of the first tree header.
     *
     * @param page_number The page number of the first tree header.
     * @return A reference to this builder to support a fluent interface.
     */
    FileHeaderBuilder& WithFirstTreeHeaderPage(PageNumber page_number);

    /**
     * @brief Sets or overwrites the page number of the first freelist page.
     *
     * @param page_number The page number of the first freelist page.
     * @return A reference to this builder to support a fluent interface.
     */
    FileHeaderBuilder& WithFirstFreeListPage(PageNumber page_number);
};

}

#endif //NOID_SRC_BACKEND_PAGE_DATABASEHEADER_H_

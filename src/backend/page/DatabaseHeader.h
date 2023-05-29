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

/**
 * @brief The default page size in bytes.
 */
uint16_t const DEFAULT_PAGE_SIZE = 4096;

/**
 * @brief The default key size in bytes.
 */
uint8_t const DEFAULT_KEY_SIZE = 16;

class DatabaseHeaderBuilder;

/**
 * @brief The noid database header format.
 *
 * @author houthacker
 */
class DatabaseHeader {
 public:
    /**
     * @brief The size in bytes of a noid database header on disk.
     */
    static uint8_t const BYTE_SIZE = 100;

 private:
    friend class DatabaseHeaderBuilder;

    std::array<byte, DatabaseHeader::BYTE_SIZE> const data;

    /**
     * @brief Creates a new @c DatabaseHeader.
     *
     * @param data The raw backing data.
     */
    explicit DatabaseHeader(std::array<byte, DatabaseHeader::BYTE_SIZE> data);

 public:

    /**
     * @brief Creates a new builder for @c DatabaseHeader instances, using default values.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<DatabaseHeaderBuilder> NewBuilder();

    /**
     * @brief Creates a new builder for @c DatabaseHeader instances, using the given raw array as a base.
     * @details The data in this array is expanded to
     *
     * @param base The data to use as a basis for the new @c DatabaseHeader instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<DatabaseHeaderBuilder> NewBuilder(std::array<byte, DatabaseHeader::BYTE_SIZE> &base);

    /**
     * @brief Creates a new builder for @c DatabaseHeader instances, using @p base as a starting point.
     *
     * @param base The @c DatabaseHeader to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<DatabaseHeaderBuilder> NewBuilder(const DatabaseHeader& base);

    /**
     * @return An unmodifiable reference to the raw bytes of this header.
     */
    [[nodiscard]] const std::array<byte, DatabaseHeader::BYTE_SIZE>& GetBytes() const;

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
     * @details The header signature is used to verify the header has not been corrupted.
     * @return The header signature.
     */
    [[nodiscard]] uint32_t GetSignature() const;

    /**
     * @details Two instances of @c DatabaseHeader are considered equal if their serialized form contains the same
     * bytes.
     *
     * @param other The instance to compare to.
     * @return Whether this @c DatabaseHeader is considered equal to @p other.
     */
    [[nodiscard]] bool Equals(const DatabaseHeader& other) const;
};

bool operator==(const DatabaseHeader& lhs, const DatabaseHeader& rhs);
bool operator!=(const DatabaseHeader& lhs, const DatabaseHeader& rhs);

/**
 * @brief Builder for @c DatabaseHeader instances.
 *
 * @author houthacker
 */
class DatabaseHeaderBuilder {
 private:
    friend class DatabaseHeader;

    uint16_t page_size;
    uint8_t key_size;
    PageNumber first_tree_header_page;
    PageNumber first_freelist_page;

    explicit DatabaseHeaderBuilder();
    explicit DatabaseHeaderBuilder(std::array<byte, DatabaseHeader::BYTE_SIZE> const & base);

    /**
     * @brief Creates a new builder with default values.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<DatabaseHeaderBuilder> Create();

    /**
     * @brief Creates a new builder with a copy of the the given backing array.
     *
     * @param base The backing array to use as a basis for the new header instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the checksum verification fails.
     */
    static std::unique_ptr<DatabaseHeaderBuilder> Create(std::array<byte, DatabaseHeader::BYTE_SIZE> const & base);

    /**
     * @brief Creates a new builder with a copy of the data in @p base.
     *
     * @param base The header to use as a basis for the new header instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<DatabaseHeaderBuilder> Create(const DatabaseHeader& base);
 public:

    /**
     * @brief Creates a new @c DatabaseHeader instance based on the provided data.
     *
     * @return The new @c DatabaseHeader instance.
     */
    [[nodiscard]] std::unique_ptr<const DatabaseHeader> Build() const;

    /**
     * @brief Sets or overwrites the currently configured page size.
     * @details If @p page_size is less than 512 or not a power of 2, it is set to respectively 512 or
     * the next power of two.
     *
     * @param size The new page size.
     * @return A reference to this builder to support a fluent interface.
     */
    DatabaseHeaderBuilder& WithPageSize(uint16_t size = DEFAULT_PAGE_SIZE);

    /**
     * @brief Sets or overwrites the currently configured key size.
     * @details If @p key_size is less than, or not a multiple of 8, it will be rounded to the
     * next multiple of 8.
     *
     * @param size The new key size.
     * @return A reference to this builder to support a fluent interface.
     */
    DatabaseHeaderBuilder& WithKeySize(uint8_t size = DEFAULT_KEY_SIZE);

    /**
     * @brief Sets or overwrites the page number of the first tree header.
     *
     * @param page_number The page number of the first tree header.
     * @return A reference to this builder to support a fluent interface.
     */
    DatabaseHeaderBuilder& WithFirstTreeHeaderPage(PageNumber page_number);

    /**
     * @brief Sets or overwrites the page number of the first freelist page.
     *
     * @param page_number The page number of the first freelist page.
     * @return A reference to this builder to support a fluent interface.
     */
    DatabaseHeaderBuilder& WithFirstFreeListPage(PageNumber page_number);
};

}

#endif //NOID_SRC_BACKEND_PAGE_DATABASEHEADER_H_

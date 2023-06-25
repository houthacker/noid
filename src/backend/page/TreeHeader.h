/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_TREEHEADER_H_
#define NOID_SRC_BACKEND_PAGE_TREEHEADER_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "backend/DynamicArray.h"
#include "backend/Types.h"

namespace noid::backend::page {

class TreeHeaderBuilder;

enum class TreeType {

    /**
     * The 'default' tree type. Using this anywhere except for an initial value will cause an exception somewhere
     * down the road.
     */
    None = 0x0000,

    /**
     * Table b+trees contain the actual data, and are identified within a serialized @c TreeHeader by the
     * type @c 'TT', which is @c 0x5454 in hexadecimal notation.
     */
    Table = 0x5454,

    /**
     * Index b+trees contain no data in their leaf nodes, but instead point to it. In a serialized @c TreeHeader
     * they are identified by @c 'TI', which is @c 0x4954 in hexadecimal notation.
     */
    Index = 0x4954,
};

/**
 * @brief Implementation of the tree header page.
 * @details A @c TreeHeader contains the metadata required to locate and operate on table- or index trees.
 *
 * @see The serialized-format description in the linked documentation at doc/database_file_formats.md
 *
 * @author houthacker
 */
class TreeHeader {
 public:
    static const uint8_t HEADER_SIZE = 14;

 private:
    friend class TreeHeaderBuilder;

    /**
     * @brief The type of tree this is the header of.
     */
    const TreeType tree_type;

    /**
     * @brief The maximum amount of entries an @c InternalNode may contain.
     * @details This is a calculated value which is redundantly stored in the file format as well.
     * In a future release, this value might be removed from the file format, but currently it
     * is used when validating a subset of the page types when retrieved from persistent storage.
     */
    const uint16_t max_node_entries;

    /**
     * @brief The maximum amount of records a @c LeafNode may contain.
     * @details This is a calculated value which is redundantly stored in the file format as well.
     * In a future release, this value might be removed from the file format, but currently it
     * is used when validating a subset of the page types when retrieved from persistent storage.
     */
    const uint16_t max_node_records;

    /**
     * The number of the page at which the root node is stored.
     */
    const PageNumber root;

    /**
     * The number of pages contained in this b+tree. This includes this @c TreeHeader itself.
     */
    const uint32_t page_count;

    /**
     * The size in bytes of @c TreeHeader page. The page size is required when serializing a @c TreeHeader instance,
     * but is itself not serialized into the @c TreeHeader. Instead, the page size is stored in the serialized
     * @c FileHeader of a database.
     */
     const uint16_t page_size;

    TreeHeader(TreeType tree_type, uint16_t max_node_entries, uint16_t max_node_records, PageNumber root,
        uint32_t page_count, uint16_t page_size);

 public:

    /**
     * @brief Creates a new builder for @c TreeHeader instances.
     *
     * @param page_size The page size in bytes.
     * @return The new builder instance.
     * @throws std::length_error if @p page_size is too small to contain a @c TreeHeader.
     */
    static std::shared_ptr<TreeHeaderBuilder> NewBuilder(uint16_t page_size);

    /**
     * @brief Creates a new builder for @c TreeHeader instances, using @c base as a starting point.
     *
     * @param base The @c TreeHeader to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::shared_ptr<TreeHeaderBuilder> NewBuilder(const TreeHeader& base);

    /**
     * @brief Creates a new builder for @c TreeHeader instances, using @c base as a starting point.
     *
     * @param base The raw bytes to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw data do not represent a valid @c TreeHeader.
     * @throws std::length_error if the raw data is a valid @c TreeHeader, but the maximum node entries or maximum
     * node records are not supported using the page size used by the current @c Pager.
     */
    static std::shared_ptr<TreeHeaderBuilder> NewBuilder(DynamicArray<byte>&& base);

    /**
     * @return The type of b+tree this is the header for.
     */
    [[nodiscard]] TreeType GetTreeType() const;

    /**
     * @details This value is calculated as <code>(page size - internal node page header size) / entry size</code>.
     * When using default values, this boils down to <code>(uint16_t)(4096 - 24) / 20 => 203</code>.
     *
     * @return The maximum amount of entries in internal nodes.
     */
    [[nodiscard]] uint16_t GetMaxInternalEntries() const;

    /**
     * @details This value is calculated as <code>(page size - leaf node header size) / record size</code>.
     * When using default values, this boils down to <code>(uint16_t)(4096 - 24) / 24 => 169</code>.
     *
     * @return The maximum amount of records in leaf nodes.
     */
    [[nodiscard]] uint16_t GetMaxLeafRecords() const;

    /**
     * @return The page number of the root node in the database file.
     */
    [[nodiscard]] PageNumber GetRoot() const;

    /**
     * @return The number of pages contained in this b+tree, including this @c TreeHeader.
     */
    [[nodiscard]] uint32_t GetPageCount() const;

    /**
     * @brief Serializes this instance to a dynamic byte array.
     *
     * @return The serialized @c TreeHeader.
     */
    [[nodiscard]] DynamicArray<byte> ToBytes() const;
};

 class TreeHeaderBuilder : public std::enable_shared_from_this<TreeHeaderBuilder> {
 private:
    friend class TreeHeader;

    /**
     * @brief The configured page size in bytes. Defaults to @c NoidConfig::vfs_page_size.
     */
    uint16_t page_size;

    /**
     * @brief The type of tree.
     */
    TreeType tree_type;

    /**
     * @brief The maximum amount of entries an @c InternalNode may contain.
     */
    uint16_t max_node_entries;

    /**
     * @brief The maximum amount of records a @c LeafNode may contain.
     */
    uint16_t max_node_records;

    /**
     * The number of the page at which the root node is stored.
     */
    PageNumber root;

    /**
     * The number of pages contained in this b+tree. This includes this @c TreeHeader itself.
     */
    uint32_t page_count;

    explicit TreeHeaderBuilder(uint16_t size);
    explicit TreeHeaderBuilder(const TreeHeader& base);
    explicit TreeHeaderBuilder(DynamicArray<byte>&& base);

 public:

    TreeHeaderBuilder() =delete;

    /**
     * @brief Creates a new @c TreeHeader based on the provided data.
     *
     * @return The new @c TreeHeader instance.
     * @throws std::domain_error if the tree type or the root page has not been set.
     */
    [[nodiscard]] std::unique_ptr<const TreeHeader> Build();

    /**
     * @brief Sets the type of b+tree this header is for.
     *
     * @param type The tree type.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::domain_error if the tree type has been set before and is different than @p type.
     */
    std::shared_ptr<TreeHeaderBuilder> WithTreeType(TreeType type);

    /**
     * @brief Sets the page number of the root node of this tree. If not set, it defaults to 0 (meaning no page).
     *
     * @param root_page The page number to set.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::domain_error if the root page has been set already and differs from @p root_page.
     */
    std::shared_ptr<TreeHeaderBuilder> WithRootPageNumber(PageNumber root_page);

    /**
     * @brief Sets the amount of pages in the containing b+tree (this header included).
     *
     * @param count The amount of pages.
     * @return  A reference to this builder to support a fluent interface.
     * @throws std::out_of_range if @p count is zero.
     */
    std::shared_ptr<TreeHeaderBuilder> WithPageCount(uint32_t count);

    /**
     * @brief Increments the page count with the given amount.
     *
     * @param amount The increment value.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::overflow_error if adding @p amount to the current page count would wrap around the page count value.
     */
    std::shared_ptr<TreeHeaderBuilder> IncrementPageCount(uint32_t amount);
};

}

#endif //NOID_SRC_BACKEND_PAGE_TREEHEADER_H_

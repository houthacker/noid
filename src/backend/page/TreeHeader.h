/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_TREEHEADER_H_
#define NOID_SRC_BACKEND_PAGE_TREEHEADER_H_

#include <cstdint>
#include <memory>
#include <vector>
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
 private:
    friend class TreeHeaderBuilder;

    /**
     * @brief The type of tree this is the header of.
     */
    TreeType tree_type;

    /**
     * @brief The maximum amount of entries an @c InternalNode may contain.
     * @details This is a calculated value which is redundantly stored in the file format as well.
     * In a future release, this value might be removed from the file format, but currently it
     * is used when validating a subset of the page types when retrieved from persistent storage.
     */
    uint16_t max_node_entries;

    /**
     * @brief The maximum amount of records a @c LeafNode may contain.
     * @details This is a calculated value which is redundantly stored in the file format as well.
     * In a future release, this value might be removed from the file format, but currently it
     * is used when validating a subset of the page types when retrieved from persistent storage.
     */
    uint16_t max_node_records;

    /**
     * The number of the page at which the root node is stored.
     */
    PageNumber root;

    explicit TreeHeader(TreeType tree_type, uint16_t max_node_entries, uint16_t max_node_records, PageNumber root);

 public:

    /**
     * @brief Creates a new builder for @c TreeHeader instances, using an all-zeroes backing vector.
     *
     * @return The new builder instance.
     */
    static std::unique_ptr<TreeHeaderBuilder> NewBuilder();

    /**
     * @brief Creates a new builder for @c TreeHeader instances, using @c base as a starting point.
     *
     * @param base The @c TreeHeader to use as a basis for the new instance.
     * @return The new builder instance.
     */
    static std::unique_ptr<TreeHeaderBuilder> NewBuilder(const TreeHeader& base);

    /**
     * @brief Creates a new builder for @c TreeHeader instances, using @c base as a starting point.
     *
     * @param base The raw data to use as a basis for the new instance.
     * @return The new builder instance.
     * @throws std::invalid_argument if the given raw data is not a valid @c TreeHeader.
     */
    static std::unique_ptr<TreeHeaderBuilder> NewBuilder(std::vector<byte>&& base);

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
};

class TreeHeaderBuilder {
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

    explicit TreeHeaderBuilder();
    explicit TreeHeaderBuilder(const TreeHeader& base);
    explicit TreeHeaderBuilder(std::vector<byte>&& base);

 public:

    /**
     * @brief Creates a new @c TreeHeader based on the provided data.
     *
     * @return The new @c TreeHeader instance.
     * @throws std::domain_error if the tree type or the root page has not been set.
     */
    [[nodiscard]] std::unique_ptr<const TreeHeader> Build();

    /**
     * @brief Sets the type of b+tree this header is for.
     * @param type
     * @return A reference to this builder to support a fluent interface.
     * @throws std::domain_error if the tree type has been set before and is different than @p type.
     */
    TreeHeaderBuilder& WithTreeType(TreeType type);

    /**
     * @brief Sets the page number of the root node of this tree. If not set, it defaults to 0 (meaning no page).
     *
     * @param root_page The page number to set.
     * @return A reference to this builder to support a fluent interface.
     * @throws std::domain_error if the root page has been set already and differs from @p root_page.
     */
    TreeHeaderBuilder& WithRootPageNumber(PageNumber root_page);
};

}

#endif //NOID_SRC_BACKEND_PAGE_TREEHEADER_H_

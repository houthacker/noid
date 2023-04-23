#ifndef NOID_SRC_STORAGE_BPLUSTREE_H_
#define NOID_SRC_STORAGE_BPLUSTREE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "Shared.h"
#include "BPlusTreeNode.h"
#include "BPlusTreeLeafNode.h"
#include "BPlusTreeInternalNode.h"

namespace noid::storage {

class BPlusTree {
 private:

    /**
     * The tree order is used to determine the minimum- and maximum amount of
     * entries a node is allowed to have. This in turn helps determine when a node
     * needs to be split, merged with- or redistributed between itself and a sibling.
     * The following min/max values (inclusive) exist per type of node:
     * <ul>
     * <li>Root node: min = 1, max = order * 2</li>
     * <li>Internal node: min = order, max = order * 2</li>
     * <li>Leaf node: min = order, max = order * 2</li>
     * <ul>
     */
    uint8_t  order;

    /**
     * The root node is @c nullptr before the first insert.
     */
    std::shared_ptr<BPlusTreeNode> root;

    /**
     * @brief Recursively finds the leaf having a key range containing the given @p key, starting with @p node.
     * @details The returned leaf is the only leaf that can contain the given @p key if it exists in this tree. However,
     * since the search is executed using a range match, it must be checked if the leaf actually contains the @p key if
     * this is a requirement. Use @c BPlusTreeNode::Contains(key) for this.
     *
     * @param node The search entry point.
     * @param key The search key.
     * @return A reference to the leaf node.
     */
    std::shared_ptr<BPlusTreeLeafNode> FindLeafRangeMatch(std::shared_ptr<BPlusTreeNode> node, const K& key);

    /**
     * @brief Recursively finds the node(s) containing the given @p key, starting at @p node.
     * @details Searches the tree for the internal- and leaf node containing the search key. The internal node is
     * returned if it contains the key, while the leaf node is returned when its range implies that it
     * should contain the key. However, if an internal node is returned with this search, the leaf node
     * contains the key as well.
     *
     * @param node The search entry point.
     * @param key The search key.
     * @return The found nodes.
     */
    std::pair<std::shared_ptr<BPlusTreeInternalNode>, std::shared_ptr<BPlusTreeLeafNode>> FindNodes(std::shared_ptr<BPlusTreeNode> node, const K& key);

 public:

    /**
     * @brief Creates a new @c BPlusTree with an order of at least @c BTREE_MIN_ORDER.
     *
     * @param order The order of the tree.
     * @throws std::invalid_argument If order is less than @c BTREE_MIN_ORDER.
     */
    explicit BPlusTree(uint8_t order);
    ~BPlusTree()= default;

    /**
     * @return An unmanaged pointer to the root node.
     */
    BPlusTreeNode* Root();

    /**
     * @brief Inserts the given key/value pair into this tree, overwriting any pre-existing value having
     * the same key.
     *
     * @param key The key to later retrieve the value with.
     * @param value The actual data to be stored.
     * @return The type of insert.
     */
    InsertType Insert(const K& key, V& value);

    /**
     * @brief Removes the given value from the tree and returns its associated value.
     *
     * @param key The key to remove.
     * @return The associated value, or an empty optional if no such record exists.
     */
    std::optional<V> Remove(const K& key);

    /**
     * @brief Writes a textual representation of this tree to the given stream.
     *
     * @param out The stream to Write the output to.
     */
    void Write(std::stringstream& out);
};

}

#endif //NOID_SRC_STORAGE_BPLUSTREE_H_

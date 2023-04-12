#ifndef NOID_SRC_STORAGE_BPLUSTREE_H_
#define NOID_SRC_STORAGE_BPLUSTREE_H_

#include <cstdint>
#include <memory>

#include "Shared.h"
#include "BPlusTreeNode.h"
#include "BPlusTreeLeafNode.h"

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
    std::unique_ptr<BPlusTreeNode> root;

    /**
     * @brief Recursively finds the leaf in which the given @p key should reside, starting with @p node.
     *
     * @param node The search entry point.
     * @param key The search key.
     * @return A reference to the leaf node.
     */
    BPlusTreeLeafNode& FindLeaf(BPlusTreeNode& node, const K& key);

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
};

}

#endif //NOID_SRC_STORAGE_BPLUSTREE_H_

#ifndef NOID_SRC_STORAGE_BPLUSTREELEAFNODE_H_
#define NOID_SRC_STORAGE_BPLUSTREELEAFNODE_H_

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "BPlusTreeNode.h"
#include "BPlusTreeRecord.h"
#include "Shared.h"

namespace noid::storage {

class BPlusTreeLeafNode : public BPlusTreeNode {
 private:

    /**
     * The tree order. This is used to determine the min/max amount of keys for this node.
     * @see BPlusTree::order
     */
    uint8_t order;

    /**
     * The records in this node. Records contain the actual data next to the related search key.
     */
    std::vector<std::unique_ptr<BPlusTreeRecord>> records;

    /**
     * The parent node. If @c nullptr, this node is the root node.
     */
    BPlusTreeInternalNode* parent;

    /**
     * The left sibling node containing records considered less than any record in this node. May be @c nullptr.
     */
    BPlusTreeLeafNode* previous;

    /**
     * The right sibling node containing records considered greater than any record in this node. May be @c nullptr.
     */
    BPlusTreeLeafNode* next;

    /**
     * @brief Copies the given key to the parent node, creating a new parent if this is the root.
     *
     * @param key The key to insert into the parent.
     * @return Whether the tree was grown to facilitate copying up.
     */
    bool CopyUp(const K& key);

 public:

    /**
     * @brief Creates a new BPlusTreeLeafNode.
     * @details A record must be added in order to ensure the node is never empty, except just prior to
     * merging it with another node.
     *
     * @param parent The parent node, which may be @c nullptr
     * @param order The tree order.
     * @param record The first record.
     */
    BPlusTreeLeafNode(BPlusTreeInternalNode* parent, uint8_t order, std::unique_ptr<BPlusTreeRecord> record);
    ~BPlusTreeLeafNode() override = default;

    /**
     * @return Whether this node is the containing trees' root node.
     */
    bool IsRoot() override;

    /**
     * @return Whether this node contains more than the maximum amount of keys.
     */
    bool IsFull() override;

    /**
     * @return The parent node, or @c nullptr if this is the root node.
     */
    BPlusTreeInternalNode* Parent() override;

    /**
     * @return The left sibling, or @c nullptr if no such node exists.
     */
    BPlusTreeLeafNode* Previous();

    /**
     * @return The right sibling, or @c nullptr if no such node exists.
     */
    BPlusTreeLeafNode* Next();

    /**
     * @return The smallest key
     */
    const K& SmallestKey();

    /**
     * @brief Sets the parent of this node to the given one.
     *
     * @param p The new parent node. May be @c nullptr
     */
    void SetParent(BPlusTreeInternalNode* p) override;

    /**
     * @brief Copies @p key and @p value and inserts them into this node.
     * @details If the key already exists, the pre-existing value is overwritten.
     *
     * @param key The key to insert.
     * @param value The related data.
     * @return @c true if inserting the key/value increased the size of this node.
     */
    bool Insert(const K& key, V& value);

    /**
     * @brief Redistributes the node keys evenly between this node and a newly created sibling, copying up
     * the middle key.
     * @details If the node contains less than @c BTREE_MIN_ORDER elements, this method does nothing but
     * return @c SplitSideEffect::None.
     *
     * @return The side effect this split has on the containing tree.
     */
    SplitSideEffect Split() override;
};

}

#endif //NOID_SRC_STORAGE_BPLUSTREELEAFNODE_H_

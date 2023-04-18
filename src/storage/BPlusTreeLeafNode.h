#ifndef NOID_SRC_STORAGE_BPLUSTREELEAFNODE_H_
#define NOID_SRC_STORAGE_BPLUSTREELEAFNODE_H_

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
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
     * @brief Returns whether this node could take the records from the given sibling before getting full.
     *
     * @param sibling The sibling to possibly merge with.
     * @return Whether this node can merge with the given @p sibling.
     */
    bool IsMergeableWith(BPlusTreeLeafNode& sibling);

    /**
     * @brief Copies the given key to the parent node, creating a new parent if this is the root.
     *
     * @param key The key to insert into the parent.
     * @return Whether the tree was grown to facilitate copying up.
     */
    bool CopyUp(const K& key);

    /**
     * @brief Redistributes the records between itself and its left- or right sibling.
     * @details This redistribution is done between leaf nodes and their shared parent, which is an internal node.
     * This results in slightly different behaviour than redistribution between just internal nodes since
     * redistribution between leaf- and internal nodes also deals with the actual data. Generically speaking, only the
     * keys are redistributed.
     *
     * @return Whether any records were distributed.
     */
    bool Redistribute();

    /**
     * @brief Merges the records of this node with its left- or right sibling.
     * @details Merging two nodes also removes the parent key which has the merged-to node as its right child.
     * If so required, the indicated change in tree structure is used by the containing tree to replace the
     * (then empty) root node by the merged child.
     *
     * @return The change to the tree structure that occurred during this merge.
     */
    TreeStructureChange Merge();

    /**
     * @brief Removes the smallest record from this node and returns it.
     * @details This method assumes that this node is rich. Calling this method when it is not rich yields a @c nullptr.
     *
     * @return The smallest record of this node, or a null pointer if this record is not rich.
     */
    std::unique_ptr<BPlusTreeRecord> TakeSmallest();

    /**
     * @brief Removes the largest record from this node and returns it.
     * @details This method assumes that this node is rich. Calling this method when it is not rich yields a @c nullptr.
     *
     * @return The largest record of this node, or a null pointer if this record is not rich.
     */
    std::unique_ptr<BPlusTreeRecord> TakeLargest();

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
     * @return Whether this node contains less than the minimum amount of keys, including zero.
     */
    bool IsPoor() override;

    /**
     * @return Whether this node contains more than tme minimum amount of records. Full nodes are rich as well.
     */
    bool IsRich() override;

    /**
     * @param key The search key.
     * @return Whether this node contains the given key.
     */
    bool Contains(const K& key) override;

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
     * @return The smallest key.
     */
    const K& SmallestKey();

    /**
     * @return The greatest key.
     */
    const K& LargestKey();

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
     * return @c TreeStructureChange::None.
     *
     * @return The side effect this split has on the containing tree.
     */
    TreeStructureChange Split() override;

    /**
     * @brief Removes the given key and if such record exists in this node, returns its value.
     * @param key The key to remove.
     * @return The record value or an empty optional if no such record exists.
     */
    std::optional<V> Remove(const K& key);

    /**
     * @brief Rearranges the records contained in this node, its siblings and their common parent node.
     *
     * @param removed The key whose removal caused this rearrangement.
     * @return Any significant side effect of this rearrangement.
     */
    TreeStructureChange Rearrange(const K& removed) override;
};

}

#endif //NOID_SRC_STORAGE_BPLUSTREELEAFNODE_H_

#ifndef NOID_SRC_STORAGE_BPLUSTREEINTERNALNODE_H_
#define NOID_SRC_STORAGE_BPLUSTREEINTERNALNODE_H_

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "BPlusTreeNode.h"
#include "BPlusTreeKey.h"
#include "Shared.h"

namespace noid::storage {

class BPlusTreeInternalNode : public BPlusTreeNode {
 private:

    /**
     * The tree order. This is used to determine the min/max amount of keys for this node.
     * @see BPlusTree::order
     */
    uint8_t order;

    /**
     * The keys contained in this node.
     */
    std::vector<std::unique_ptr<BPlusTreeKey>> keys;

    /**
     * The parent node. If @c nullptr, this node is the root node.
     */
    BPlusTreeInternalNode* parent;

    /**
     * @brief Moves the given @p key to the parent node.
     * @details If so required, this method creates and attaches a new parent node to contain the given @p key.
     *
     * @param key The key to push up.
     * @return true if the tree height has been increased, false otherwise.
     */
    bool PushUp(std::unique_ptr<BPlusTreeKey> key);

    /**
     * @brief Inserts the given @p key and sorts the keys afterwards to ensure the correct order.
     *
     * @param key The key to insert.
     */
    void InsertInternal(std::unique_ptr<BPlusTreeKey> key);

    /**
     * Internal constructor to support @c BPlusTreeInternalNode::Split() and @c BPlusTreeInternalNode::PushUp().
     *
     * @param parent The parent node, or @c nullptr.
     * @param order The tree order.
     * @param keys The initial set of keys.
     */
    BPlusTreeInternalNode(BPlusTreeInternalNode* parent, uint8_t order, std::vector<std::unique_ptr<BPlusTreeKey>> keys);

 public:

    /**
     * @brief Creates a new @c BPlusTreeInternalNode
     * @details Inserts the given @p key, @p left_child and @p right_child as a new @c BPlusTreeKey. Since a
     * @c BPlusTreeInternalNode can only exist if there are leaf child nodes, at least one of @p left_child and
     * @p right_child must have a value. When adding the child nodes, their parent instance is set to this node.
     *
     * @param parent The parent of this node, or @c nullptr if this is the root node.
     * @param order The tree order.
     * @param key The search key.
     * @param left_child The left child, containing the lesser elements.
     * @param right_child The right child, containing the equal- and greater elements.
     * @throws std::invalid_argument If both @p left_child and @p right_child are @c nullptr.
     */
    BPlusTreeInternalNode(BPlusTreeInternalNode* parent, uint8_t order, const K& key, BPlusTreeNode* left_child,
                          BPlusTreeNode* right_child);

    ~BPlusTreeInternalNode() override = default;

    /**
     * @return Whether this node is the containing trees' root node
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
     * @return The smallest key in this node.
     */
    BPlusTreeKey* Smallest();

    /**
     * @brief Returns the largest @c BPlusInternalKey which is less than or equal to the given @p key.
     *
     * @param key The search key.
     * @return The largest @c BPlusInternalKey that not exceeds @p key.
     */
    BPlusTreeKey* GreatestNotExceeding(const K& key);

    /**
     * @brief Sets the parent of this node to the given one.
     * @details If @c nullptr is provided, this method does nothing.
     *
     * @param parent The new parent node.
     */
    void SetParent(BPlusTreeInternalNode* parent) override;

    /**
     * @brief Creates and inserts a new @c BPlusTreeKey based on the given key and children.
     * @details If a key with the given @p key already exists in this node,
     * this method does nothing but return @c false. If the key does not exist, a new @c BPlusTreeKey is created,
     * and inserted. Any adjacent keys will inherit the children from this key.
     *
     * @param key The key.
     * @param left_child The left child node, which may be @c nullptr.
     * @param right_child  The right child node, which may be @c nullptr.
     * @return Whether the key was inserted.
     */
    bool Insert(const K& key, BPlusTreeNode* left_child, BPlusTreeNode* right_child);

    /**
     * @brief Redistributes the node keys evenly between this node and a newly created sibling, pushing up
     * the middle key.
     * @details If the node contains less than @c BTREE_MIN_ORDER elements, this method does nothing but
     * return @c SplitSideEffect::None.
     *
     * @return The side effect this split has on the containing tree.
     */
    SplitSideEffect Split() override;
};

}

#endif //NOID_SRC_STORAGE_BPLUSTREEINTERNALNODE_H_

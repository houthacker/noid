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

 class BPlusTreeInternalNode : public BPlusTreeNode, public std::enable_shared_from_this<BPlusTreeInternalNode> {
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
    std::shared_ptr<BPlusTreeInternalNode> parent;

    /**
     * @return The left sibling, or @c nullptr if no such node exists.
     */
    std::shared_ptr<BPlusTreeInternalNode> LeftSibling();

    /**
     * @return The right sibling, or @c nullptr if no such node exists.
     */
    std::shared_ptr<BPlusTreeInternalNode> RightSibling();

    /**
     * @brief Returns whether this node could take the keys from the given sibling before getting full.
     *
     * @param sibling The sibling to possibly merge with.
     * @return Whether this node can merge with the given @p sibling.
     */
    bool IsMergeableWith(BPlusTreeInternalNode& sibling);

    /**
     * @brief Moves the given @p key to the parent node.
     * @details If so required, this method creates and attaches a new parent node to contain the given @p key.
     *
     * @param key The key to push up.
     * @return true if the tree height has been increased, false otherwise.
     */
    bool PushUp(std::unique_ptr<BPlusTreeKey> key);

    /**
     * @brief Redistributes the keys between itself, its parent and its left- or right sibling.
     *
     * @return Whether any keys were distributed.
     */
    bool Redistribute();

    /**
     * @brief Merges the keys of this node with its left- or right sibling.
     * @details Merging two nodes also removes the 'middle' parent key with points to both merged nodes.
     *
     * If so required, the indicated change in tree structure is used by the containing tree to replace the
     * (then empty) root node by the merged child.
     *
     * @return The change to the tree structure that occurred during this merge.
     */
    TreeStructureChange Merge();

    /**
     * @brief Inserts the given @p key and sorts the keys afterwards to ensure the correct order.
     *
     * @param key The key to insert.
     */
    void InsertInternal(std::unique_ptr<BPlusTreeKey> key);

    /**
     * @brief Removes the largest key from this node and returns it for later usage.
     *
     * @return The largest key of this node.
     */
    std::unique_ptr<BPlusTreeKey> TakeLargest();

    /**
     * @brief Removes the smallest key from this node and returns it for later usage.
     *
     * @return The smallest key of this node.
     */
    std::unique_ptr<BPlusTreeKey> TakeSmallest();

    /**
     * @brief Removes the key which points to @p left as its left child and @p right as its right child and returns
     * it for later usage.
     *
     * @param left The left child node of the key.
     * @param right The right child node of the key.
     * @return The middle key, or @c nullptr if no such key exists.
     */
    std::unique_ptr<BPlusTreeKey> TakeMiddle(BPlusTreeInternalNode& left, BPlusTreeInternalNode& right);

    /**
     * Internal constructor to support the Create factory methods.
     *
     * @param parent The parent node, or @c nullptr.
     * @param order The tree order.
     */
    BPlusTreeInternalNode(std::shared_ptr<BPlusTreeInternalNode> parent, uint8_t order);

    /**
     * @brief Creates a new @c BPlusTreeInternalNode whose pointer is managed by the wrapping @c std::shared_ptr.
     * @details Additionally adopts the given @p keys by iterating over them and setting the parent of its children
     * to the new @c BPlusTreeInternalNode.
     *
     * @param parent The parent of this node, or @c nullptr if this is the root node.
     * @param order The tree order.
     * @param keys The keys to adopt.
     * @return The new internal node.
     */
    static std::shared_ptr<BPlusTreeInternalNode> Create(std::shared_ptr<BPlusTreeInternalNode> parent, uint8_t order, std::vector<std::unique_ptr<BPlusTreeKey>> keys);

 public:

    /**
     * @brief Creates a new @c BPlusTreeInternalNode whose pointer is managed by the wrapping @c std::shared_ptr.
     * @details Inserts the given @p key, @p left_child and @p right_child as a new @c BPlusTreeKey. Since a
     * @c BPlusTreeInternalNode can only exist if there are leaf child nodes, at least one of @p left_child and
     * @p right_child must have a value. When adding the child nodes, their parent instance is set to this node.
     *
     * @param parent The parent of this node, or @c nullptr if this is the root node.
     * @param order The tree order.
     * @param key The search key.
     * @param left_child The left child, containing the lesser elements.
     * @param right_child The right child, containing the equal- and greater elements.
     * @return the new internal node.
     */
     static std::shared_ptr<BPlusTreeInternalNode> Create(std::shared_ptr<BPlusTreeInternalNode> parent, uint8_t order, const K& key,
                                                        std::shared_ptr<BPlusTreeNode> left_child, std::shared_ptr<BPlusTreeNode> right_child);

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
     * @return Whether this node contains no keys.
     */
    bool IsEmpty();

    /**
     * @return Whether this node contains less than the minimum amount of keys, including zero.
     */
    bool IsPoor() override;

    /**
     * @return Whether this node contains more than tme minimum amount of keys. Full nodes are rich as well.
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
    std::shared_ptr<BPlusTreeInternalNode> Parent() override;

    /**
     * @return The smallest key in this node.
     */
    BPlusTreeKey* Smallest();

    /**
     * @brief Returns the largest @c BPlusTreeKey which is less than or equal to the given @p key.
     *
     * @param key The search key.
     * @return The largest @c BPlusTreeKey that not exceeds @p key.
     */
    BPlusTreeKey* GreatestNotExceeding(const K& key);

    /**
     * @brief Returns the @c BPlusTreeKey with the immediate next largest key compared to the given @p key.
     *
     * @param key The search key.
     * @return The next largest key, or @c nullptr if no such key exists.
     */
    BPlusTreeKey* NextLargest(const K& key);

    /**
     * @brief Sets the parent of this node to the given one.
     * @details If @c nullptr is provided, this method does nothing.
     *
     * @param parent The new parent node.
     */
    void SetParent(std::shared_ptr<BPlusTreeInternalNode> parent) override;

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
    bool Insert(const K& key, std::shared_ptr<BPlusTreeNode> left_child, std::shared_ptr<BPlusTreeNode> right_child);

    /**
     * @brief Redistributes the node keys evenly between this node and a newly created sibling, pushing up
     * the middle key.
     * @details If the node contains less than @c BTREE_MIN_ORDER elements, this method does nothing but
     * return @c SplitSideEffect::None.
     *
     * @return The side effect this split has on the containing tree.
     */
    TreeStructureChange Split() override;

    /**
     * @brief Removes the given key and returns whether the size of this node changed as a result.
     *
     * @param key The key to remove.
     * @return Whether ths node size changed due to this removal.
     */
    bool Remove(const K& key);

    /**
     * @brief Rearranges the records contained in this node, its siblings and their common parent node.
     *
     * @param removed The key whose removal caused this rearrangement.
     * @return Any significant side effect of this rearrangement.
     */
    TreeStructureChange Rearrange(const K& removed) override;

    /**
     * @brief Writes a textual representation of this node to the given stream.
     *
     * @param out The stream to Write the output to.
     */
    void Write(std::stringstream& out) override;
};

}

#endif //NOID_SRC_STORAGE_BPLUSTREEINTERNALNODE_H_

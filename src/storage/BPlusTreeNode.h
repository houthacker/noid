#ifndef NOID_SRC_STORAGE_BPLUSTREENODE_H_
#define NOID_SRC_STORAGE_BPLUSTREENODE_H_

#include "Shared.h"

namespace noid::storage {

class BPlusTreeInternalNode;

/**
 * @brief Defines shared behaviour between internal- and leaf nodes.
 */
class BPlusTreeNode {

 public:
    BPlusTreeNode()= default;

    virtual ~BPlusTreeNode()= default;

    /**
     * @return Whether this node is the root node.
     */
    virtual bool IsRoot()= 0;

    /**
     * @return Whether this node contains more than the maximum amount of keys.
     */
    virtual bool IsFull()= 0;

    /**
     * @return The parent node, or @c nullptr if this is the root node.
     */
    virtual BPlusTreeInternalNode* Parent()= 0;

    /**
     * @brief Sets the parent of this node to the given one.
     *
     * @param parent The new parent node. May be @c nullptr
     */
    virtual void SetParent(BPlusTreeInternalNode* parent)= 0;

    /**
     * @brief Redistributes the keys or records evenly between itself and a new sibling.
     * @details If the node contains less than @c BTREE_MIN_ORDER elements, this method does nothing but
     * return @c SplitSideEffect::None.
     *
     * @return The caused side effect of this split.
     */
    virtual SplitSideEffect Split()= 0;
};

}

#endif //NOID_SRC_STORAGE_BPLUSTREENODE_H_

#ifndef NOID_SRC_STORAGE_BPLUSTREENODE_H_
#define NOID_SRC_STORAGE_BPLUSTREENODE_H_

#include <memory>
#include <sstream>

#include "Shared.h"
#include "Rearrangement.h"

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
     * @return Whether this node contains more than the maximum amount of entries.
     */
    virtual bool IsFull()= 0;

    /**
     * @return Whether this node contains less than the minimum amount of entries, including zero.
     */
    virtual bool IsPoor()= 0;

    /**
     * @return Whether this node contains more than tme minimum amount of entries. Full nodes are rich as well.
     */
    virtual bool IsRich()= 0;

    /**
     * @param key The search key.
     * @return Whether this node contains the given key.
     */
    virtual bool Contains(const K& key)= 0;

    /**
     * @return The parent node, or @c nullptr if this is the root node.
     */
    virtual std::shared_ptr<BPlusTreeInternalNode> Parent()= 0;

    /**
     * @brief Sets the parent of this node to the given one.
     *
     * @param parent The new parent node. May be @c nullptr
     */
    virtual void SetParent(std::shared_ptr<BPlusTreeInternalNode> parent)= 0;

    /**
     * @brief Redistributes the keys or records evenly between itself and a new sibling.
     * @details If the node contains less than @c BTREE_MIN_ORDER elements, this method does nothing but
     * return @c TreeStructureChange::None.
     *
     * @return Any significant side effect of this split.
     */
    virtual TreeStructureChange Split()= 0;

    /**
     * @brief Rearranges the entries contained in this node, its siblings and their common parent.
     *
     * @return Any significant side effect of this rearrangement.
     */
    virtual Rearrangement Rearrange()= 0;

    /**
     * @brief Writes a textual representation of this node and its children to the given stream.
     *
     * @param out The stream to Write the output to.
     */
    virtual void Write(std::stringstream& out)= 0;
};

}

#endif //NOID_SRC_STORAGE_BPLUSTREENODE_H_

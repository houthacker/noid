#ifndef NOID_SRC_ALGORITHM_BPLUSTREENODE_H_
#define NOID_SRC_ALGORITHM_BPLUSTREENODE_H_

#include <memory>
#include <optional>
#include <sstream>

#include "Shared.h"

namespace noid::algorithm {

class BPlusTreeNode;
class BPlusTreeInternalNode;

/**
 * @brief Describes the available types of entry rearrangement.
 */
enum class RearrangementType {
    /**
     * No rearrangement was executed.
     */
    None,

    /**
     * Rearrangement was done by merging entries of two nodes and their common parent.
     */
    Merge,

    /**
     * Rearrangement was done by redistributing entries between two existing nodes and their common parent.
     */
    Redistribute,

    /**
     * Rearrangement was done by splitting the subject node and evenly distributing the entries between the nodes
     * and their common parent.
     */
    Split,
};

/**
 * @brief Short-lived structure to contain the rearrangement results.
 */
struct EntryRearrangement {
    /**
     * @brief The type of rearrangement.
     */
    const RearrangementType type;

    /**
     * If @c type is @c RearrangementType::Merge, the subject is the merged-into node. If @c type is
     * @c RearrangementType::Split, the subject is the new node.
     */
    const std::optional<std::shared_ptr<BPlusTreeNode>> subject;
};

/**
 * @brief Defines shared behaviour between internal- and leaf nodes.
 */
class BPlusTreeNode {

 public:
    BPlusTreeNode() = default;

    virtual ~BPlusTreeNode() = default;

    /**
     * @return Whether this node is the root node.
     */
    virtual bool IsRoot() = 0;

    /**
     * @return Whether this node contains more than the maximum amount of entries.
     */
    virtual bool IsFull() = 0;

    /**
     * @return Whether this node contains less than the minimum amount of entries, including zero.
     */
    virtual bool IsPoor() = 0;

    /**
     * @return Whether this node contains more than tme minimum amount of entries. Full nodes are rich as well.
     */
    virtual bool IsRich() = 0;

    /**
     * @param key The search key.
     * @return Whether this node contains the given key.
     */
    virtual bool Contains(const K& key) = 0;

    /**
     * @return The parent node, or @c nullptr if this is the root node.
     */
    virtual std::shared_ptr<BPlusTreeInternalNode> Parent() = 0;

    /**
     * @brief Sets the parent of this node to the given one.
     *
     * @param parent The new parent node. May be @c nullptr
     */
    virtual void SetParent(std::shared_ptr<BPlusTreeInternalNode> parent) = 0;

    /**
     * @brief Rearranges the entries contained in this node, its siblings and their common parent.
     *
     * @return The executed type of rearrangement.
     */
    [[nodiscard]] virtual EntryRearrangement Rearrange() = 0;

    /**
     * @brief Writes a textual representation of this node and its children to the given stream.
     *
     * @param out The stream to Write the output to.
     */
    virtual void Write(std::stringstream& out) = 0;
};

}

#endif //NOID_SRC_ALGORITHM_BPLUSTREENODE_H_

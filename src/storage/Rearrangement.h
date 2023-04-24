#ifndef NOID_SRC_STORAGE_REARRANGEMENT_H_
#define NOID_SRC_STORAGE_REARRANGEMENT_H_

#include <memory>
#include <optional>

namespace noid::storage {

// forward declare BPlusTreeNode to prevent cyclic references
class BPlusTreeNode;

/**
 * @brief Describes the type of rearrangement that was executed.
 */
enum class RearrangementType {

    /**
     * @brief Rearrangement was done by merging entries of two nodes and their common parent.
     */
    Merge,

    /**
     * @brief Rearrangement was done by redistribution entries between two nodes and their common parent.
     */
    Redistribution,
};

/**
 * @brief Short-lived structure to contain the rearrangement results.
 */
struct Rearrangement {
    /**
     * @brief The type of rearrangement.
     */
    const RearrangementType type;

    /**
     * @brief If @c type is @c RearrangementType::Merge, this field contains the merged-into node.
     */
    const std::optional<std::shared_ptr<BPlusTreeNode>> merged_into;
};

}

#endif //NOID_SRC_STORAGE_REARRANGEMENT_H_

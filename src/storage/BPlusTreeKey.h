#ifndef NOID_SRC_STORAGE_BPLUSTREEKEY_H_
#define NOID_SRC_STORAGE_BPLUSTREEKEY_H_

#include <memory>

#include "BPlusTreeNode.h"
#include "KeyBearer.h"
#include "Shared.h"

namespace noid::storage {

/**
 * @brief A container for a key and the child nodes it points to.
 */
class BPlusTreeKey : public KeyBearer {
 private:
    K key;

 public:

    /**
     * @brief Creates a new @c BPlusTreeKey based on the given @p key.
     * @param key The actual key.
     */
    explicit BPlusTreeKey(K key);
    BPlusTreeKey()= delete;
    BPlusTreeKey(BPlusTreeKey const&)= delete;
    BPlusTreeKey(BPlusTreeKey &&)= default;
    ~BPlusTreeKey() override = default;

    BPlusTreeKey& operator=(BPlusTreeKey const&)= delete;
    BPlusTreeKey& operator=(BPlusTreeKey &&)= default;

    /**
     * @brief The left child containing lesser keys. May be @c nullptr
     */
    std::shared_ptr<BPlusTreeNode> left_child;

    /**
     * @brief The right child containing the equal- and greater keys. May be @c nullptr
     */
    std::shared_ptr<BPlusTreeNode> right_child;

    /**
     * @return A reference to the key.
     */
    [[nodiscard]] const K& Key() const override;

    /**
     * @brief Replaces the current key by copying in the given one. The previous key is discarded.
     *
     * @param replacement The replacement key.
     */
    void Replace(const K& replacement);
};

/**
 * @brief Compares the given @c BPlusTreeKey instances on equality.
 * @details Two @c BPlusTreeKey instances are considered equal if their @c key fields are equal.
 *
 * @param lhs The left hand side.
 * @param rhs The right hand side.
 * @return Whether the given keys are considered equal.
 */
bool operator==(const BPlusTreeKey& lhs, const BPlusTreeKey& rhs);

/**
 * @brief Determines if @p lhs is considered less than @p rhs.
 * @details The comparison is done using the @c key fields of the given @c BPlusTreeKey instances.
 *
 * @param lhs The left hand side.
 * @param rhs The right hand side.
 * @return Whether the left hand key is considered less than the right hand key.
 */
bool operator<(const BPlusTreeKey &lhs, const BPlusTreeKey& rhs);

}

#endif //NOID_SRC_STORAGE_BPLUSTREEKEY_H_

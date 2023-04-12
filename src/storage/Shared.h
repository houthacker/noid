#ifndef NOID_SRC_STORAGE_SHARED_H_
#define NOID_SRC_STORAGE_SHARED_H_

#include <array>
#include <vector>
#include <cstdint>

namespace noid::storage {

/**
 * The minimal order of a @c BPlusTree. This is required to ensure the properties of a b(plus)tree always hold.
 * For example, splitting a node with 1 element would violate the 50%-rule.
 */
const uint8_t BTREE_MIN_ORDER = 2;

/**
 * The key size in bytes of all keys in a @c BPlusTree.
 */
const uint8_t BTREE_KEY_SIZE = 16;
using byte = unsigned char;

/**
 * Alias for the search key type.
 */
using K = std::array<byte, BTREE_KEY_SIZE>;

/**
 * Alias for the value type.
 */
using V = std::vector<byte>;

/**
 * @brief Describes any possible side effect of splitting a node.
 */
enum class SplitSideEffect {

    /**
     * Splitting the node had no side effect.
     */
    None,

    /**
     * Splitting the node caused the creation of a new root node.
     */
    NewRoot,
};

/**
 * @brief Describes the available types of insert.
 */
enum class InsertType {

    /**
     * The data was inserted using a new slot within the node.
     */
    Insert,

    /**
     * Pre-existing data was overwritten on insert, and no new slot was used to store it.
     */
    Upsert,
};

}

#endif //NOID_SRC_STORAGE_SHARED_H_

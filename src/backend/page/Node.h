/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_NODE_H
#define NOID_NODE_H

#include "backend/Types.h"

namespace noid::backend::page {

/**
 * @brief Interface describing shared behaviour between @c InternalNode and @c LeafNode classes.
 * @details Noid preferably uses concepts to describe behaviour, but that is not possible in this case, since the
 * concrete type of root node changes at runtime between implementations.
 *
 * @author houthacker
 */
class Node {
 public:

    virtual ~Node() =default;

    /**
     * @param key The identifier to search.
     * @return Whether this node contains the given id.
     */
    [[nodiscard]] virtual bool Contains(const SearchKey& key) const =0;
};

}

#endif //NOID_NODE_H

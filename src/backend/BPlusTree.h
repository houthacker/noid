/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_BPLUSTREE_H_
#define NOID_SRC_BACKEND_BPLUSTREE_H_

#include <memory>

#include "backend/concurrent/Concepts.h"
#include "backend/page/DatabaseHeader.h"

#include "Pager.h"

namespace noid::backend {

template<Lockable Lockable, SharedLockable SharedLockable>
class BPlusTree {
 private:
    std::unique_ptr<Pager<Lockable, SharedLockable>> pager;
    std::unique_ptr<const page::DatabaseHeader> header;
};

}

#endif //NOID_SRC_BACKEND_BPLUSTREE_H_

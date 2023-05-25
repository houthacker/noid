/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_VFS_VFSFACTORY_H_
#define NOID_SRC_BACKEND_VFS_VFSFACTORY_H_

#include <memory>

#include "backend/NoidConfig.h"
#include "backend/concurrent/Concepts.h"
#include "NoidVFS.h"

using namespace noid::backend::concurrent;

namespace noid::backend::vfs {

template<Lockable Lockable, SharedLockable SharedLockable>
class VFSFactory {
 private:
    VFSFactory() =default;
    ~VFSFactory() =default;

 public:

    /**
     * @brief Creates a new VFS instance.
     *
     * @param config The noid configuration singleton.
     * @return The new VFS instance, or @c nullptr if the configured VFS type cannot be created on the runtime platform.
     */
    static std::unique_ptr<NoidVFS<Lockable, SharedLockable>> Create(const NoidConfig& config);
};

}

#endif //NOID_SRC_BACKEND_VFS_VFSFACTORY_H_

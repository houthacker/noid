/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */


#include "VFSFactory.h"
#include "unix/UnixVFS.h"

namespace noid::backend::vfs {

template<Lockable Lockable, SharedLockable SharedLockable>
std::unique_ptr<NoidVFS<Lockable, SharedLockable>> noid::backend::vfs::VFSFactory<Lockable, SharedLockable>::Create(const noid::backend::NoidConfig &config) {
  switch (config.vfs_type) {
    case VFSType::Unix: return std::make_unique<UnixVFS>(); break;
  }

  return {nullptr};
}

}

/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */


#include "UnixVFS.h"
#include "UnixFile.h"

namespace noid::backend::vfs {

std::shared_ptr<NoidFile<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>>> UnixVFS::Open(const fs::path &path) {
  return UnixFile::Open(path);
}

std::shared_ptr<NoidFile<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>>> UnixVFS::CreateTempFile() {
  return UnixFile::CreateTempFile();
}

}
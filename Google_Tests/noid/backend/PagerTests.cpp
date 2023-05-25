/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "backend/Pager.h"
#include "backend/vfs/VFSFactory.h"
#include "backend/NoidConfig.h"

using namespace noid::backend;

class PagerFixture : public ::testing::Test {
 protected:
    std::shared_ptr<vfs::NoidFile> temp_file;

    void SetUp() override {
      auto& cfg = NoidConfig::Get();
      cfg.vfs_type = VFSType::Memory;

      temp_file = vfs::VFSFactory::Create(cfg)->CreateTempFile();
    }
};

TEST_F(PagerFixture, FileExistsAfterOpen) {
  auto pager = Pager::Open(temp_file);
  EXPECT_NE(pager, nullptr) << "Expect a new Pager instance";
}
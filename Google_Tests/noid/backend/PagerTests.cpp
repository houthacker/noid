/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "backend/Pager.h"

#include <cstdio>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

using namespace noid::backend;

class PagerFixture : public ::testing::Test {
 protected:
    fs::path temp_file;

    void SetUp() override {
      temp_file = fs::path(std::tmpnam(nullptr));
    }

    void TearDown() override {
      if (fs::exists(temp_file)) {
        std::error_code ec;
        if (!fs::remove(temp_file, ec)) {
          std::cout << "***** Could not remove test file " << temp_file.string() << " *****" << std::endl;
        }
      }
    }
};

TEST_F(PagerFixture, FileExistsAfterOpen) {
  auto pager = Pager::Open(temp_file);
  EXPECT_NE(pager, nullptr) << "Expect a new Pager instance";
}
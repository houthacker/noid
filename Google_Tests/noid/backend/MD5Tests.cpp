#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "backend/MD5.h"
#include <string>

using ::testing::StrEq;
using namespace noid::backend;

class MD5Fixture : public ::testing::Test {

};

TEST_F(MD5Fixture, EmptyStringDigest) {
  auto empty_string = std::string("");
  auto md5 = MD5::Digest(empty_string);

  auto expected = std::string("d41d8cd98f00b204e9800998ecf8427e");
  EXPECT_THAT(md5.ToHexString(), StrEq(expected));
}

TEST_F(MD5Fixture, TheQuickBrownFox) {
  auto empty_string = std::string("The quick brown fox jumps over the lazy dog");
  auto md5 = MD5::Digest(empty_string);

  auto expected = std::string("9e107d9d372bb6826bd81d3542a419d6");
  EXPECT_THAT(md5.ToHexString(), StrEq(expected));
}
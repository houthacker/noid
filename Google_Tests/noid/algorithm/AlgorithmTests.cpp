#include "gtest/gtest.h"

#include <memory>
#include <vector>

#include "algorithm/Algorithm.h"
#include "algorithm/BPlusTreeKey.h"
#include "algorithm/Shared.h"

using namespace noid::algorithm;

static const K& GetKeyReference(const BPlusTreeKey& key) {
  return key.Key();
}

class AlgorithmFixture : public ::testing::Test {};

TEST_F(AlgorithmFixture, NextLargest) {
  // Prepare
  K key1 = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
  K key2 = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5};
  K key3 = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12};
  K key4 = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18};

  std::vector<std::unique_ptr<BPlusTreeKey>> vec;
  vec.push_back(std::make_unique<BPlusTreeKey>(key1));
  vec.push_back(std::make_unique<BPlusTreeKey>(key2));
  vec.push_back(std::make_unique<BPlusTreeKey>(key3));
  vec.push_back(std::make_unique<BPlusTreeKey>(key4));

  // Execute
  K needle = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  auto index = noid::algorithm::NextLargest(vec, 0, static_cast<int64_t>(vec.size() - 1), needle, GetKeyReference);
  EXPECT_EQ(index, 0) << "Expect next-largest element to reside at index 0 for smaller needle";

  needle[BTREE_KEY_SIZE - 1] = 3;
  index = noid::algorithm::NextLargest(vec, 0, static_cast<int64_t>(vec.size() - 1), needle, GetKeyReference);
  EXPECT_EQ(index, 1) << "Expect next-largest element to reside at index 1 for small-medium needle";

  needle[BTREE_KEY_SIZE - 1] = 10;
  index = noid::algorithm::NextLargest(vec, 0, static_cast<int64_t>(vec.size() - 1), needle, GetKeyReference);
  EXPECT_EQ(index, 2) << "Expect next-largest element to reside at index 2 for medium needle";

  needle[BTREE_KEY_SIZE - 1] = 15;
  index = noid::algorithm::NextLargest(vec, 0, static_cast<int64_t>(vec.size() - 1), needle, GetKeyReference);
  EXPECT_EQ(index, 3) << "Expect next-largest element to reside at index 3 for medium-large needle";

  needle[BTREE_KEY_SIZE - 1] = 19;
  index = noid::algorithm::NextLargest(vec, 0, static_cast<int64_t>(vec.size() - 1), needle, GetKeyReference);
  EXPECT_EQ(index, -1) << "Expect next-largest element to not exist for larger needle";
}
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <sstream>
#include <iostream>

#include "storage/BPlusTree.h"
#include "storage/BPlusTreeInternalNode.h"

using ::testing::ContainerEq;
using namespace noid::storage;

class BPlusTreeFixture : public ::testing::Test {
 protected:
    BPlusTree* tree;

    void SetUp() override {
      tree = new BPlusTree(BTREE_MIN_ORDER);
    }

    void TearDown() override {
      delete tree;
    }
};

TEST_F(BPlusTreeFixture, Upsert) {
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value_one = {1, 3, 3, 7};
  V value_two = {1, 3, 3, 8};

  EXPECT_EQ(tree->Insert(key, value_one), InsertType::Insert);
  EXPECT_EQ(tree->Insert(key, value_two), InsertType::Upsert);
}

TEST_F(BPlusTreeFixture, Splitting) {
  K key_base = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value_base = {1, 3, 3, 0};

  for (auto i = 0; i <= BTREE_MIN_ORDER * 2; i++) {
    K key = key_base;
    key[BTREE_KEY_SIZE - 1] = i;

    V value = value_base;
    value[3] = i;

    EXPECT_EQ(tree->Insert(key, value), InsertType::Insert);
  }

  // After inserting BTREE_MIN_ORDER * 2 distinct items, the initial root must have been split,
  // therefore the root node must be a BPlusTreeInternalNode and not a BPlusTreeLeafNode.
  EXPECT_NE(dynamic_cast<BPlusTreeInternalNode*>(tree->Root()), nullptr) << "Expect root to be a BPlusTreeInternalNode after a split";

  std::stringstream buf;
  tree->Write(buf);

  EXPECT_STREQ(buf.str().c_str(), "[2]\n[0* 1*] [2* 3* 4*]\n");
}

TEST_F(BPlusTreeFixture, Redistribution) {
  K key_base = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value = {1, 3, 3, 7};

  std::vector<byte> key_values = {2, 5, 12, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 29};
  for (auto b : key_values) {
    K key = key_base;
    key[BTREE_KEY_SIZE - 1] = b;

    tree->Insert(key, value);
  }

  std::stringstream buf;
  tree->Write(buf);

  auto expected_after_setup =
      "[17]\n"
      "[12 15] [19 21 23 25]\n"
      "[2* 5*] [12* 13*] [15* 16*] [17* 18*] [19* 20*] [21* 22*] [23* 24*] [25* 26* 27* 29*]\n";
  EXPECT_STREQ(buf.str().c_str(), expected_after_setup);

  auto remove_key = key_base;
  remove_key[BTREE_KEY_SIZE - 1] = 24;
  auto removed = tree->Remove(remove_key);

  EXPECT_TRUE(removed.has_value()) << "Expect a removed value";
  EXPECT_THAT(removed.value(), ContainerEq(value));

  buf = std::stringstream();
  tree->Write(buf);

  auto expected_after_redistribution =
      "[17]\n"
      "[12 15] [19 21 23 27]\n"
      "[2* 5*] [12* 13*] [15* 16*] [17* 18*] [19* 20*] [21* 22*] [23* 26*] [27* 29*]\n";
  EXPECT_STREQ(buf.str().c_str(), expected_after_redistribution) << "Expect redistribution of [23* 24*] [25* 26* 27* 29*] -> [23* 26*] [27* 29*] after removal of 24*";
}

TEST_F(BPlusTreeFixture, Merger) {
  K key_base = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value = {1, 3, 3, 7};

  std::vector<byte> key_values = {2, 5, 12, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 29};
  for (auto b : key_values) {
    K key = key_base;
    key[BTREE_KEY_SIZE - 1] = b;

    tree->Insert(key, value);
  }

  std::stringstream buf;
  tree->Write(buf);

  auto expected_after_setup =
      "[17]\n"
      "[12 15] [19 21 23 25]\n"
      "[2* 5*] [12* 13*] [15* 16*] [17* 18*] [19* 20*] [21* 22*] [23* 24*] [25* 26* 27* 29*]\n";
  EXPECT_STREQ(buf.str().c_str(), expected_after_setup);

  // Removing key [..., 20] must merge [17* 18*] with [19* <removed>*], since this tree tries to merge with left
  // siblings first. This also causes the removal of key 19 from their parent node [19 21 23 25].
  auto remove_key = key_base;
  remove_key[BTREE_KEY_SIZE - 1] = 20;
  auto removed = tree->Remove(remove_key);

  EXPECT_TRUE(removed.has_value()) << "Expect a removed value";
  EXPECT_THAT(removed.value(), ContainerEq(value));

  buf = std::stringstream();
  tree->Write(buf);

  auto expected_after_merge =
      "[17]\n"
      "[12 15] [21 23 25]\n"
      "[2* 5*] [12* 13*] [15* 16*] [17* 18* 19*] [21* 22*] [23* 24*] [25* 26* 27* 29*]\n";
  EXPECT_STREQ(buf.str().c_str(), expected_after_merge) << "Expect merge to leave tree in valid state";
}
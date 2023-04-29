#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "algorithm/BPlusTreeLeafNode.h"
#include "algorithm/BPlusTreeInternalNode.h"

using ::testing::ContainerEq;
using namespace noid::algorithm;

class BPlusTreeLeafNodeFixture : public ::testing::Test {};

TEST_F(BPlusTreeLeafNodeFixture, NoParentMeansRoot) {
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value = {1, 3, 3, 7};
  auto node = BPlusTreeLeafNode::Create(nullptr, 3, std::make_unique<BPlusTreeRecord>(key, value));

  EXPECT_EQ(node->Parent(), nullptr) << "Expect the parent node to be nullptr";
  EXPECT_TRUE(node->IsRoot()) << "Expect a node without parent to identify as the root node";
}

TEST_F(BPlusTreeLeafNodeFixture, Saturate) {
  auto order = 3;
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value = {1, 3, 3, 7};
  auto node = BPlusTreeLeafNode::Create(nullptr, order, std::make_unique<BPlusTreeRecord>(key, value));

  for (auto i = 0; i <= order * 2; i++) {
    auto k = key;
    k[BTREE_KEY_SIZE - 1] = i + 1;

    EXPECT_TRUE(node->Insert(k, value)) << "Expect insert #" << +i << " to increase node size";
  }

  EXPECT_TRUE(node->IsFull()) << "Expect " << +(order * 2) << " inserts to cause a full node.";
}

TEST_F(BPlusTreeLeafNodeFixture, Contains) {
  auto order = 3;
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value = {1, 3, 3, 7};
  auto node = BPlusTreeLeafNode::Create(nullptr, order, std::make_unique<BPlusTreeRecord>(key, value));

  for (auto i = 0; i < order; i++) {
    auto k = key;
    k[BTREE_KEY_SIZE - 1] = i + 1;

    node->Insert(k, value);
  }

  // Verification
  auto second_key = key;
  second_key[BTREE_KEY_SIZE - 1] = 1;

  auto non_existing_key = key;
  non_existing_key[BTREE_KEY_SIZE - 1] = order + 1;

  EXPECT_TRUE(node->Contains(second_key)) << "Expect node to contain inserted key";
  EXPECT_FALSE(node->Contains(non_existing_key)) << "Expect node to report that is doesn't contain a non-inserted key";
}

TEST_F(BPlusTreeLeafNodeFixture, SmallestKey) {
  auto order = 3;
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (byte)(order * 2)};
  V value = {1, 3, 3, 7};
  auto node = BPlusTreeLeafNode::Create(nullptr, order, std::make_unique<BPlusTreeRecord>(key, value));

  // Insert in reverse order. If the keys were not sorted, this test will fail.
  for (auto i = (order * 2) - 1; i >= 0; i--) {
    auto k = key;
    k[BTREE_KEY_SIZE - 1] = i;

    EXPECT_TRUE(node->Insert(k, value)) << "Expect insert #" << +i << " to increase node size";
  }

  const K expected_smallest_key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  EXPECT_EQ(node->SmallestKey(), expected_smallest_key);
}

TEST_F(BPlusTreeLeafNodeFixture, InsertKeyTwiceOverwritesPrevious) {
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value = {1, 3, 3, 7};
  auto node = BPlusTreeLeafNode::Create(nullptr, 3, std::make_unique<BPlusTreeRecord>(key, value));

  EXPECT_FALSE(node->Insert(key, value)) << "Expect no increase in node size on 2nd insert of same key";
}

TEST_F(BPlusTreeLeafNodeFixture, Split) {
  auto order = 3;
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  V value = {1, 3, 3, 7};
  auto node = BPlusTreeLeafNode::Create(nullptr, order, std::make_unique<BPlusTreeRecord>(key, value));

  // Fill up the node
  for (auto i = 0; i <= order * 2; i++) {
    auto k = key;
    k[BTREE_KEY_SIZE - 1] = i + 1;

    node->Insert(k, value);
  }
  EXPECT_TRUE(node->IsFull()) << "Expect a full node before splitting";
  EXPECT_EQ(node->Parent(), nullptr) << "Expect node to have no parent before split";

  // Execute the split.
  EXPECT_EQ(node->Rearrange().type, RearrangementType::Split) << "Expect node rearrangement splits it";
  auto parent = node->Parent();
  EXPECT_NE(parent, nullptr) << "Expect an actual non-null parent after a split";
  EXPECT_TRUE(parent->IsRoot()) << "Expect the parent node to be the new root";

  // Parent node must contain the smallest key of new sibling.
  auto sibling = node->Next();
  EXPECT_NE(sibling, nullptr) << "Expect a right sibling after splitting";
  EXPECT_EQ(sibling->Previous(), node) << "Expect BPlusTreeLeafNode::previous of new sibling to be the original node.";
  EXPECT_EQ(node->Parent(), sibling->Parent()) << "Expect siblings to share the same parent";
  EXPECT_EQ(sibling->SmallestKey(), parent->Smallest()->Key()) << "Expect smallest key from sibling to have been copied up to the parent";

  auto parent_key = parent->Smallest();
  EXPECT_EQ(parent_key->left_child, node) << "Left child of parent key must refer to original node";
  EXPECT_EQ(parent_key->right_child.get(), sibling.get()) << "Right child of parent key must refer to sibling node";
}
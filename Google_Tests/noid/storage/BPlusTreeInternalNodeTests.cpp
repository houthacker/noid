#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "storage/BPlusTreeInternalNode.h"
#include "storage/BPlusTreeLeafNode.h"

using namespace noid::storage;

#ifndef NOID_CREF
#define NOID_CREF(x) reinterpret_cast<const (x)&>(x)
#endif

class BPlusTreeInternalNodeFixture : public ::testing::Test {

};

TEST_F(BPlusTreeInternalNodeFixture, NoParentMeansRoot) {
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  auto node = BPlusTreeInternalNode(nullptr, BTREE_MIN_ORDER, key, nullptr, nullptr);

  EXPECT_EQ(node.Parent(), nullptr) << "Expect the parent node to be nullptr";
  EXPECT_TRUE(node.IsRoot()) << "Expect a node without parent to identify as the root node";
}

TEST_F(BPlusTreeInternalNodeFixture, Saturate) {
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t order = BTREE_MIN_ORDER;
  auto node = BPlusTreeInternalNode(nullptr, order, key, nullptr, nullptr);

  for (auto i = 0; i <= order * 2; i++) {
    auto k = key;
    k[BTREE_KEY_SIZE - 1] = i + 1;

    EXPECT_TRUE(node.Insert(k, nullptr, nullptr)) << "Expect insert #" << +i << " to increase node size";
  }

  EXPECT_TRUE(node.IsFull()) << "Expect " << +(order * 2) << " inserts to cause a full node.";
}

TEST_F(BPlusTreeInternalNodeFixture, SmallestKey) {
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t order = BTREE_MIN_ORDER;
  auto node = BPlusTreeInternalNode(nullptr, order, key, nullptr, nullptr);

  // Insert in reverse order. If the keys were not sorted, this test will fail, since smallest gets us
  // the key at index 0.
  for (auto i = (order * 2) - 1; i > 0; i--) {
    auto k = key;
    k[BTREE_KEY_SIZE - 1] = i;

    EXPECT_TRUE(node.Insert(k, nullptr, nullptr)) << "Expect insert #" << +i << " to increase node size";
  }

  const BPlusTreeKey expected_smallest_key = BPlusTreeKey({57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
  EXPECT_EQ(reinterpret_cast<const BPlusTreeKey&>(*node.Smallest()), reinterpret_cast<const BPlusTreeKey&>(expected_smallest_key));
}

TEST_F(BPlusTreeInternalNodeFixture, KeyCanBeInsertedOnlyOnce) {
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t order = BTREE_MIN_ORDER;
  auto node = BPlusTreeInternalNode(nullptr, order, key, nullptr, nullptr);

  EXPECT_FALSE(node.Insert(key, nullptr, nullptr)) << "Expect inserting a key for the second time should not have any effect";
}

TEST_F(BPlusTreeInternalNodeFixture, Split) {
  K key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t order = BTREE_MIN_ORDER;
  auto node = BPlusTreeInternalNode(nullptr, order, key, nullptr, nullptr);

  for (auto i = 0; i <= order * 2; i++) {
    K k = key;
    k[BTREE_KEY_SIZE - 1] = i + 1;

    node.Insert(k, nullptr, nullptr);
  }
  EXPECT_TRUE(node.IsFull()) << "Expect node to be full before split";
  EXPECT_EQ(node.Parent(), nullptr) << "Expect node to be parent before split";

  // Execute the split
  EXPECT_EQ(node.Split(), SplitSideEffect::NewRoot) << "Expect splitting the node creates a new parent";

  // Verify
  auto parent = node.Parent();
  EXPECT_NE(parent, nullptr) << "Expect to have an actual new parent";
  EXPECT_TRUE(parent->IsRoot());

  // Middle key must have been pushed up
  const K expected_middle_key = {57, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3};
  const K& expected_ref = expected_middle_key;

  auto parent_key = parent->Smallest();
  EXPECT_EQ(parent_key->Key(), expected_ref);

  // Sibling and node must share the same parent
  auto sibling = parent_key->right_child.get();
  EXPECT_EQ(parent, sibling->Parent());

  // parent_key must refer to both node and sibling
  EXPECT_EQ(parent_key->left_child.get(), &node);
  EXPECT_EQ(parent_key->right_child.get(), sibling);
}
#include "BPlusTree.h"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include "BPlusTreeInternalNode.h"

namespace noid::storage {

static inline bool IsInternalNode(BPlusTreeNode& node) {
  if (dynamic_cast<BPlusTreeInternalNode*>(&node)) {
    return true;
  }

  return false;
}

static inline uint8_t EnsureMinOrder(uint8_t value) {
  if (value >= BTREE_MIN_ORDER) {
    return value;
  }

  std::stringstream buf;
  buf << "Expect order of at least " << BTREE_MIN_ORDER << ", but got " << +value << ".";
  throw std::invalid_argument(buf.str());
}

BPlusTree::BPlusTree(uint8_t order) : order(EnsureMinOrder(order)), root(nullptr) {}

BPlusTreeLeafNode &BPlusTree::FindLeaf(BPlusTreeNode &node, const K &key) {
  if (IsInternalNode(node)) {
    auto& internal_node = reinterpret_cast<BPlusTreeInternalNode&>(node);

    // todo figure out if we can ensure these will never return nullptr.
    auto smallest = internal_node.Smallest();
    auto next = key < smallest->Key() ? smallest->left_child : internal_node.GreatestNotExceeding(key)->right_child;
    return this->FindLeaf(*next, key);
  }

  return reinterpret_cast<BPlusTreeLeafNode&>(node);
}

BPlusTreeNode *BPlusTree::Root() {
  return this->root.get();
}

InsertType BPlusTree::Insert(const K &key, V &value) {
  auto type = InsertType::Insert;
  if (this->root == nullptr) {
    this->root = std::make_unique<BPlusTreeLeafNode>(nullptr, order, std::make_unique<BPlusTreeRecord>(key, value));
    return type;
  }

  auto& leaf = this->FindLeaf(*this->root, key);
  type = leaf.Insert(key, value) ? InsertType::Insert : InsertType::Upsert;

  BPlusTreeNode* node = &leaf;
  while (node && node->IsFull()) {
    auto side_effect = node->Split();

    auto parent = node->Parent();
    if (side_effect == SplitSideEffect::NewRoot) {
      std::ignore = this->root.release(); // will be managed by the shared_ptr in BPlusTreeKey.
      this->root = std::unique_ptr<BPlusTreeNode>(parent);
    }

    node = parent;
  }

  return type;
}

}

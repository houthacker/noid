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

BPlusTreeLeafNode &BPlusTree::FindLeafRangeMatch(BPlusTreeNode &node, const K &key) {
  if (IsInternalNode(node)) {
    auto& internal_node = reinterpret_cast<BPlusTreeInternalNode&>(node);

    // todo figure out if we can ensure these will never return nullptr.
    auto smallest = internal_node.Smallest();
    auto next = key < smallest->Key() ? smallest->left_child : internal_node.GreatestNotExceeding(key)->right_child;
    return this->FindLeafRangeMatch(*next, key);
  }

  return reinterpret_cast<BPlusTreeLeafNode&>(node);
}

std::pair<BPlusTreeInternalNode *, BPlusTreeLeafNode &> BPlusTree::FindNodes(BPlusTreeNode &node, const K &key) {
  if (IsInternalNode(node)) {
    auto& internal_node = reinterpret_cast<BPlusTreeInternalNode&>(node);

    if (node.Contains(key)) {
      return {&internal_node, this->FindLeafRangeMatch(internal_node, key)};
    }

    auto smallest = internal_node.Smallest();
    auto next = key < smallest->Key() ? smallest->left_child : internal_node.GreatestNotExceeding(key)->right_child;
    return this->FindNodes(*next, key);
  }

  return {nullptr, reinterpret_cast<BPlusTreeLeafNode&>(node) };
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

  auto& leaf = this->FindLeafRangeMatch(*this->root, key);
  type = leaf.Insert(key, value) ? InsertType::Insert : InsertType::Upsert;

  BPlusTreeNode* node = &leaf;
  while (node && node->IsFull()) {
    auto side_effect = node->Split();

    auto parent = node->Parent();
    if (side_effect == TreeStructureChange::NewRoot) {
      std::ignore = this->root.release(); // will be managed by the shared_ptr in BPlusTreeKey.
      this->root = std::unique_ptr<BPlusTreeNode>(parent);
    }

    node = parent;
  }

  return type;
}

std::optional<V> BPlusTree::Remove(const K &key) {
  const auto & [internal, leaf] = this->FindNodes(*this->root, key);

  if (leaf.Contains(key)) {
    auto removed = leaf.Remove(key);

    BPlusTreeNode* node = &leaf;
    while (node && node->IsPoor()) {

      // Replace the root node if it is empty after rearrangement of entries.
      if (node->Rearrange(key) == TreeStructureChange::EmptyRoot) {
        node->SetParent(nullptr);
        this->root = std::unique_ptr<BPlusTreeNode>(node);
      }

      if (node->Parent() == internal) {
        // The key could reside in the (grand)parent of the leaf node, and is not necessarily removed
        // from it at this point. Removing it here ensures the removal happens after a possible rearrangement which
        // might get confused if the key is missing.
        // If the internal node gets poor after this, it will be rearranged in the next iteration.
        internal->Remove(key);
      }

      node = node->Parent();
    }

    return std::move(removed);
  }

  return std::nullopt;
}

}

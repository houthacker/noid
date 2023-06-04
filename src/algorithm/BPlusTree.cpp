#include "BPlusTree.h"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include "BPlusTreeInternalNode.h"

namespace noid::algorithm {

static inline bool IsInternalNode(const std::shared_ptr<BPlusTreeNode>& node)
{
  if (std::dynamic_pointer_cast<BPlusTreeInternalNode>(node)) {
    return true;
  }

  return false;
}

static inline uint8_t EnsureMinOrder(uint8_t value)
{
  if (value >= BTREE_MIN_ORDER) {
    return value;
  }

  std::stringstream buf;
  buf << "Expect order of at least " << BTREE_MIN_ORDER << ", but got " << +value << ".";
  throw std::invalid_argument(buf.str());
}

BPlusTree::BPlusTree(uint8_t order)
    :order(EnsureMinOrder(order)), root(nullptr) { }

std::shared_ptr<BPlusTreeLeafNode> BPlusTree::FindLeafRangeMatch(const std::shared_ptr<BPlusTreeNode>& node,
    const K& key)
{
  if (IsInternalNode(node)) {
    auto internal_node = std::reinterpret_pointer_cast<BPlusTreeInternalNode>(node);

    // todo figure out if we can ensure these will never return nullptr.
    auto smallest = internal_node->Smallest();
    auto next = key < smallest->Key() ? smallest->left_child : internal_node->GreatestNotExceeding(key)->right_child;
    return this->FindLeafRangeMatch(next, key);
  }

  return std::dynamic_pointer_cast<BPlusTreeLeafNode>(node);
}

std::pair<std::shared_ptr<BPlusTreeInternalNode>, std::shared_ptr<BPlusTreeLeafNode>> BPlusTree::FindNodes(
    const std::shared_ptr<BPlusTreeNode>& node, const K& key)
{
  if (IsInternalNode(node)) {
    auto internal_node = std::reinterpret_pointer_cast<BPlusTreeInternalNode>(node);

    if (node->Contains(key)) {
      return {internal_node, this->FindLeafRangeMatch(internal_node, key)};
    }

    auto smallest = internal_node->Smallest();
    auto next = key < smallest->Key() ? smallest->left_child : internal_node->GreatestNotExceeding(key)->right_child;
    return this->FindNodes(next, key);
  }

  return {nullptr, std::reinterpret_pointer_cast<BPlusTreeLeafNode>(node)};
}

BPlusTreeNode* BPlusTree::Root()
{
  return this->root.get();
}

InsertType BPlusTree::Insert(const K& key, V& value)
{
  auto type = InsertType::Insert;
  if (this->root == nullptr) {
    this->root = BPlusTreeLeafNode::Create(nullptr, order, std::make_unique<BPlusTreeRecord>(key, value));
    return type;
  }

  auto leaf = this->FindLeafRangeMatch(this->root, key);
  type = leaf->Insert(key, value) ? InsertType::Insert : InsertType::Upsert;

  BPlusTreeNode* node = leaf.get();
  while (node && node->IsFull()) {
    auto rearrangement = node->Rearrange();

    auto parent = node->Parent();
    if (rearrangement.type == RearrangementType::Split && parent->IsRoot()) {
      this->root = parent;
    }

    node = parent.get();
  }

  return type;
}

std::optional<V> BPlusTree::Remove(const K& key)
{
  const auto& [internal, leaf] = this->FindNodes(this->root, key);

  if (leaf->Contains(key)) {
    auto removed = leaf->Remove(key);

    BPlusTreeNode* node = leaf.get();
    while (node) {

      auto internal_is_parent = internal && node->Parent() == internal;
      if (internal_is_parent) {

        // The key could reside in the (grand)parent of the leaf node, and is not necessarily removed
        // from it at this point. Removing it here ensures the removal happens after a possible rearrangement which
        // might get confused if the key is missing.
        // If the internal node gets poor after this, it will be rearranged in the next iteration.
        internal->Remove(key);
      }

      auto internal_is_poor_root = internal && internal->IsRoot() && internal->IsPoor();

      // Replace the root node if it is empty after rearrangement of entries.
      if (node->IsPoor() || (internal_is_parent && internal_is_poor_root)) {
        auto rearrangement = node->Rearrange();

        if (internal_is_poor_root) {
          if (rearrangement.type == RearrangementType::Merge && rearrangement.subject.has_value()) {
            node->SetParent(nullptr);
            this->root = rearrangement.subject.value();
          }
        }
      }

      node = node->Parent().get();
    }

    return removed;
  }

  return std::nullopt;
}

void BPlusTree::Write(std::stringstream& out)
{
  if (this->root) {
    auto node = this->root;
    while (node) {
      node->Write(out);
      out << std::endl;

      if (IsInternalNode(node)) {
        auto internal = std::reinterpret_pointer_cast<BPlusTreeInternalNode>(node);
        auto smallest = internal->Smallest();

        node = smallest ? smallest->left_child : nullptr;
      }
      else {
        node = nullptr; // no layer after a leaf
      }

    }
  }
}

}

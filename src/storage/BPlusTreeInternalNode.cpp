#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "BPlusTreeInternalNode.h"
#include "Algorithm.h"

namespace noid::storage {

static const K& GetKeyReference(BPlusTreeKey& key) {
  return key.Key();
}

static bool LeftKeyIsLess(std::unique_ptr<BPlusTreeKey>& lhs, std::unique_ptr<BPlusTreeKey>& rhs) {
  return *lhs < *rhs;
}

bool BPlusTreeInternalNode::PushUp(std::unique_ptr<BPlusTreeKey> container) {
  auto must_increase_tree_height = this->IsRoot();
  if (must_increase_tree_height) {
    auto vec = std::vector<std::unique_ptr<BPlusTreeKey>>();
    vec.push_back(std::move(container));

    auto new_root = new BPlusTreeInternalNode(nullptr, this->order, std::move(vec));
    this->parent = new_root;
  } else {
    this->parent->InsertInternal(std::move(container));
  }

  return must_increase_tree_height;
}

void BPlusTreeInternalNode::InsertInternal(std::unique_ptr<BPlusTreeKey> container) {
  auto ptr = container.get();

  this->keys.push_back(std::move(container));
  std::sort(this->keys.begin(), this->keys.end(), LeftKeyIsLess);

  // Update the parent of the children
  if (ptr->left_child) {
    ptr->left_child->SetParent(this);
  }
  if (ptr->right_child) {
    ptr->right_child->SetParent(this);
  }

  // Adjacent keys inherit children from the inserted one
  auto highest_index = this->keys.size() - 1;
  auto index = BinarySearch(
      this->keys, 0, static_cast<int64_t>(this->keys.size() - 1), ptr->Key(), GetKeyReference);
  if (index > 0) {
    this->keys[index - 1]->right_child = ptr->left_child;
  }
  if (index < highest_index) {
    this->keys[index + 1]->left_child = ptr->right_child;
  }
}

BPlusTreeInternalNode::BPlusTreeInternalNode(BPlusTreeInternalNode* parent, uint8_t order, std::vector<std::unique_ptr<BPlusTreeKey>> keys)
    : parent(parent), order(order), keys(std::move(keys)) {
  for (auto& key : this->keys) {
    if (key->left_child) {
      key->left_child->SetParent(this);
    }
    if (key->right_child) {
      key->right_child->SetParent(this);
    }
  }
}

BPlusTreeInternalNode::BPlusTreeInternalNode(BPlusTreeInternalNode *parent,
                                             uint8_t order,
                                             const K &key,
                                             BPlusTreeNode *left_child,
                                             BPlusTreeNode *right_child) : parent(parent), order(order) {
  auto container = new BPlusTreeKey(key);
  container->left_child = std::shared_ptr<BPlusTreeNode>(left_child);
  container->right_child = std::shared_ptr<BPlusTreeNode>(right_child);

  if (container->left_child) {
    container->left_child->SetParent(this);
  }
  if (container->right_child) {
    container->right_child->SetParent(this);
  }

  this->keys.push_back(std::unique_ptr<BPlusTreeKey>(container));
}

bool BPlusTreeInternalNode::IsRoot() {
  return this->parent == nullptr;

}

bool BPlusTreeInternalNode::IsFull() {
  return this->keys.size() > this->order * 2;
}

BPlusTreeInternalNode *BPlusTreeInternalNode::Parent() {
  return this->parent;
}

BPlusTreeKey* BPlusTreeInternalNode::Smallest() {
  return this->keys[0].get();
}

BPlusTreeKey* BPlusTreeInternalNode::GreatestNotExceeding(const K &key) {
  auto index = noid::storage::GreatestNotExceeding(
      this->keys, 0, static_cast<int64_t>(this->keys.size() - 1), key, GetKeyReference);
  if (index >= 0) {
    return this->keys[index].get();
  }

  return nullptr;
}

void BPlusTreeInternalNode::SetParent(BPlusTreeInternalNode *p) {
  if (p && p != this) {
    this->parent = p;
  }
}

bool BPlusTreeInternalNode::Insert(const K& key, BPlusTreeNode* left_child, BPlusTreeNode* right_child) {
  auto index = BinarySearch(
      this->keys, 0, static_cast<int64_t>(this->keys.size() - 1), key, GetKeyReference);

  if (index == -1) {
    auto container = new BPlusTreeKey(key);
    container->left_child = std::shared_ptr<BPlusTreeNode>(left_child);
    container->right_child = std::shared_ptr<BPlusTreeNode>(right_child);

    this->InsertInternal(std::unique_ptr<BPlusTreeKey>(container));
    return true;
  }

  return false;
}

SplitSideEffect BPlusTreeInternalNode::Split() {
  if (this->keys.size() < BTREE_MIN_ORDER) {
    return SplitSideEffect::None;
  }

  auto middle_index = this->keys.size() / 2;

  // Remove the middle key for insertion into the parent
  auto middle_key = std::move(this->keys[middle_index]);

  // Add the largest half of the records to the new node
  auto split_keys = std::vector<std::unique_ptr<BPlusTreeKey>>();
  split_keys.reserve(this->keys.size() - middle_index - 1);

  for (auto i = middle_index + 1; i < this->keys.size(); i++) {
    split_keys.push_back(std::move(this->keys[i]));
  }

  // Remove the slots of the records that were moved to the new node.
  this->keys.resize(middle_index);

  // Create a new sibling
  auto split = new BPlusTreeInternalNode(this->parent, this->order, std::move(split_keys));

  // Set pointers on the middle key
  middle_key->left_child = std::shared_ptr<BPlusTreeNode>(this);
  middle_key->right_child = std::shared_ptr<BPlusTreeNode>(split);

  // Push up the previously removed middle key.
  if (this->PushUp(std::move(middle_key))) {
    return SplitSideEffect::NewRoot;
  }

  return SplitSideEffect::None;
}

}
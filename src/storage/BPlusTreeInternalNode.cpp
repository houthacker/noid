#include <algorithm>
#include <sstream>
#include <cstring>
#include <utility>

#include "BPlusTreeInternalNode.h"
#include "Algorithm.h"

namespace noid::storage {

static const K& GetKeyReference(BPlusTreeKey& key) {
  return key.Key();
}

static bool LeftKeyIsLess(std::unique_ptr<BPlusTreeKey>& lhs, std::unique_ptr<BPlusTreeKey>& rhs) {
  return *lhs < *rhs;
}

std::shared_ptr<BPlusTreeInternalNode> BPlusTreeInternalNode::LeftSibling() {
  if (this->IsRoot()) {
    return nullptr;
  }

  auto parent_gne = this->parent->GreatestNotExceeding(this->keys[0]->Key());
  return parent_gne ? std::dynamic_pointer_cast<BPlusTreeInternalNode>(parent_gne->left_child) : nullptr;
}

std::shared_ptr<BPlusTreeInternalNode> BPlusTreeInternalNode::RightSibling() {
  if (this->IsRoot()) {
    return nullptr;
  }

  auto parent_next_largest = this->parent->NextLargest(this->keys[this->keys.size() - 1]->Key());
  return parent_next_largest ? std::dynamic_pointer_cast<BPlusTreeInternalNode>(parent_next_largest->right_child) : nullptr;
}

bool BPlusTreeInternalNode::IsMergeableWith(BPlusTreeInternalNode &sibling) {
  return this->keys.size() + sibling.keys.size() <= this->order * 2;
}

bool BPlusTreeInternalNode::PushUp(std::unique_ptr<BPlusTreeKey> container) {
  auto must_increase_tree_height = this->IsRoot();
  if (must_increase_tree_height) {
    auto vec = std::vector<std::unique_ptr<BPlusTreeKey>>();
    vec.push_back(std::move(container));

    auto new_root = BPlusTreeInternalNode::Create(nullptr, this->order, std::move(vec));
    this->parent = new_root;
  } else {
    this->parent->InsertInternal(std::move(container));
  }

  return must_increase_tree_height;
}

bool BPlusTreeInternalNode::Redistribute() {
  if (this->IsRoot()) {
    return false; // nothing to redistribute since root has no siblings.
  }

  auto sibling = this->LeftSibling();
  if (sibling && sibling->IsRich()) {
    // Retrieve GreatestNotExceeding(my smallest) from parent
    auto parent_key = this->parent->GreatestNotExceeding(this->keys[0]->Key());

    // Prepend to our keys
    this->keys.insert(this->keys.begin(), std::make_unique<BPlusTreeKey>(parent_key->Key()));

    // Take the largest from the left sibling
    auto largest = sibling->TakeLargest();

    // Replace the parent key with the largest key we took from our sibling.
    parent_key->Replace(largest->Key());

    return true;
  }

  sibling = this->RightSibling();
  if (sibling && sibling->IsRich()) {
    // Take the smallest from the right sibling.
    auto smallest = sibling->TakeSmallest();

    // Retrieve GreatestNotExceeding(siblings smallest) from parent
    auto parent_key = this->parent->GreatestNotExceeding(smallest->Key());

    // Append the parent key to our keys
    this->keys.push_back(std::make_unique<BPlusTreeKey>(parent_key->Key()));

    // Replace the parent key value with the smallest key we took from our sibling.
    parent_key->Replace(smallest->Key());

    return true;
  }

  return false;
}

Rearrangement BPlusTreeInternalNode::Merge() {
  auto left_sibling = this->LeftSibling();
  auto right_sibling = this->RightSibling();
  std::shared_ptr<BPlusTreeInternalNode> smallest;
  std::shared_ptr<BPlusTreeInternalNode> largest;

  if (left_sibling && left_sibling->IsMergeableWith(*this)) {
    smallest = left_sibling;
    largest = shared_from_this();
  } else if (right_sibling && right_sibling->IsMergeableWith(*this)) {
    smallest = shared_from_this();
    largest = right_sibling;
  } else {
    // todo No mergeable sibling. Called out of order? Can we prove this never happens?
    return {RearrangementType::Merge, std::nullopt};
  }

  auto new_size = smallest->keys.size() + this->keys.size();

  // After determining the smallest (smaller) and largest (larger) node, merging can take place. Always merge largest
  // into smallest, because this allows appending instead of prepending. But first pull down the 'middle' key from the
  // parent, which points to both the smaller and larger node.
  auto pulled_down = largest->parent->TakeMiddle(*smallest, *largest);

  // If the parent node is empty, there is no middle key.
  if (pulled_down) {

    // And update its child pointers
    pulled_down->left_child = smallest->keys[smallest->keys.size() - 1]->right_child;
    pulled_down->right_child = largest->keys[0]->left_child;

    new_size++; // Also need to push back pulled_down key.
  }

  // Since we only end up here if redistribution fails, we assume the node we will merge with is not rich,
  // and therefore a merge with this node and the sibling will by definition not lead to a full node, since this
  // node is poor.
  smallest->keys.reserve(new_size);

  if (pulled_down) {
    smallest->keys.push_back(std::move(pulled_down));
  }

  for (auto& key : largest->keys) {
    smallest->keys.push_back(std::move(key));
  }

  return {RearrangementType::Merge, smallest};
}

void BPlusTreeInternalNode::InsertInternal(std::unique_ptr<BPlusTreeKey> container) {
  auto ptr = container.get();

  this->keys.push_back(std::move(container));
  std::sort(this->keys.begin(), this->keys.end(), LeftKeyIsLess);

  // Update the parent of the children
  if (ptr->left_child) {
    ptr->left_child->SetParent(shared_from_this());
  }
  if (ptr->right_child) {
    ptr->right_child->SetParent(shared_from_this());
  }

  // Adjacent keys inherit children from the inserted one
  auto highest_index = this->keys.size() - 1;
  auto index = BinarySearch(
      this->keys, 0, static_cast<int64_t>(this->keys.size() - 1), ptr->Key(), GetKeyReference);
  if (index > 0) {
    auto& key = this->keys[index - 1];

    if (key->right_child != ptr->left_child) {
      key->right_child = ptr->left_child;
    }
  }
  if (index < highest_index) {
    this->keys[index + 1]->left_child = ptr->right_child;
  }
}

std::unique_ptr<BPlusTreeKey> BPlusTreeInternalNode::TakeLargest() {
  if (this->keys.empty()) {
    return {nullptr};
  }

  auto index = this->keys.size() - 1;
  auto element = std::move(this->keys[index]);
  this->keys.erase(this->keys.begin() + static_cast<int64_t>(index));

  return std::move(element);
}

std::unique_ptr<BPlusTreeKey> BPlusTreeInternalNode::TakeSmallest() {
  if (this->keys.empty()) {
    return {nullptr};
  }

  auto element = std::move(this->keys[0]);
  this->keys.erase(this->keys.begin());

  return std::move(element);
}

std::unique_ptr<BPlusTreeKey> BPlusTreeInternalNode::TakeMiddle(BPlusTreeInternalNode &left,
                                                                BPlusTreeInternalNode &right) {
  auto index = noid::storage::GreatestNotExceeding(
      this->keys, 0, static_cast<int64_t>(this->keys.size() - 1), right.Smallest()->Key(), GetKeyReference);
  if (index >= 1 /* The element at index zero has no left child */ ) {
    auto candidate = this->keys[index].get();

    if (candidate->left_child.get() == &left && candidate->right_child.get() == &right) {
      auto middle = std::move(this->keys[index]);
      this->keys.erase(this->keys.begin() + index);

      return std::move(middle);
    }
  }

  return {nullptr};
}

// private, support for Split and PushUp
BPlusTreeInternalNode::BPlusTreeInternalNode(std::shared_ptr<BPlusTreeInternalNode> parent, uint8_t order)
    : parent(std::move(parent)), order(order) {}

// private
std::shared_ptr<BPlusTreeInternalNode> BPlusTreeInternalNode::Create(
    std::shared_ptr<BPlusTreeInternalNode> parent,
    uint8_t order,
    std::vector<std::unique_ptr<BPlusTreeKey>> keys) {

  auto instance = std::shared_ptr<BPlusTreeInternalNode>(new BPlusTreeInternalNode(std::move(parent), order));

  // Adopt the keys
  for (auto &key : keys) {
    if (key->left_child) {
      key->left_child->SetParent(instance);
    }
    if (key->right_child) {
      key->right_child->SetParent(instance);
    }
  }

  instance->keys = std::move(keys);
  return instance;
}

// public
std::shared_ptr<BPlusTreeInternalNode> BPlusTreeInternalNode::Create(
    std::shared_ptr<BPlusTreeInternalNode> parent,
    uint8_t order,
    const K &key,
    std::shared_ptr<BPlusTreeNode> left_child,
    std::shared_ptr<BPlusTreeNode> right_child) {

  auto instance = std::shared_ptr<BPlusTreeInternalNode>(new BPlusTreeInternalNode(std::move(parent), order));
  auto container = std::make_unique<BPlusTreeKey>(key);

  if (left_child || right_child) {
    // Create a new key and adopt it
    container->left_child = std::move(left_child);
    container->right_child = std::move(right_child);

    if (container->left_child) { container->left_child->SetParent(instance); }
    if (container->right_child) { container->right_child->SetParent(instance); }
  }

  instance->keys.push_back(std::move(container));

  return std::move(instance);
}

bool BPlusTreeInternalNode::IsRoot() {
  return this->parent == nullptr;

}

bool BPlusTreeInternalNode::IsFull() {
  return this->keys.size() > this->order * 2;
}

bool BPlusTreeInternalNode::IsEmpty() {
  return this->keys.empty();
}

bool BPlusTreeInternalNode::IsPoor() {
  return this->IsRoot() ? this->keys.empty() : this->keys.size() < this->order;
}

bool BPlusTreeInternalNode::IsRich() {
  return this->IsRoot() ? this->keys.size() > 1 : this->keys.size() > this->order;
}

bool BPlusTreeInternalNode::Contains(const K &key) {
  return BinarySearch(this->keys, 0, static_cast<int64_t>(this->keys.size() - 1),
                      key, GetKeyReference) >= 0;
}

std::shared_ptr<BPlusTreeInternalNode> BPlusTreeInternalNode::Parent() {
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

BPlusTreeKey* BPlusTreeInternalNode::NextLargest(const K &key) {
  auto index = noid::storage::NextLargest(this->keys, 0, static_cast<int64_t>(this->keys.size() - 1),
                                          key,GetKeyReference);

  if (index >= 0) {
    return this->keys[index].get();
  }

  return nullptr;
}

void BPlusTreeInternalNode::SetParent(std::shared_ptr<BPlusTreeInternalNode> p) {
  if (p && p != shared_from_this()) {
    this->parent = p;
  }
}

bool BPlusTreeInternalNode::Insert(const K& key, std::shared_ptr<BPlusTreeNode> left_child, std::shared_ptr<BPlusTreeNode> right_child) {
  auto index = BinarySearch(
      this->keys, 0, static_cast<int64_t>(this->keys.size() - 1), key, GetKeyReference);

  if (index == -1) {
    auto container = std::make_unique<BPlusTreeKey>(key);
    container->left_child = std::move(left_child);
    container->right_child = std::move(right_child);

    this->InsertInternal(std::move(container));
    return true;
  }

  return false;
}

TreeStructureChange BPlusTreeInternalNode::Split() {
  if (this->keys.size() < BTREE_MIN_ORDER) {
    return TreeStructureChange::None;
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

  // Create a new sibling.
  auto split = BPlusTreeInternalNode::Create(this->parent, this->order, std::move(split_keys));

  // Set pointers on the middle key
  middle_key->left_child = shared_from_this();
  middle_key->right_child = split;

  // Push up the previously removed middle key.
  if (this->PushUp(std::move(middle_key))) {
    return TreeStructureChange::NewRoot;
  }

  return TreeStructureChange::None;
}

bool BPlusTreeInternalNode::Remove(const K &key) {
  if (this->keys.empty()) {
    return false;
  }

  auto index = BinarySearch(
      this->keys, 0, static_cast<int64_t>(this->keys.size() - 1), key, GetKeyReference);
  if (index >= 0) {
    this->keys.erase(this->keys.begin() + index);

    return true;
  }

  return false;
}

Rearrangement BPlusTreeInternalNode::Rearrange() {
  if (this->Redistribute()) {
    return {RearrangementType::Redistribution, nullptr};
  }

  return this->Merge();
}

void BPlusTreeInternalNode::Write(std::stringstream &out) {
  out << '[';
  for (auto i = 0; i < this->keys.size(); i++) {
    auto record = this->keys[i].get();

    if (i > 0) {
      out << " ";
    }

    byte copy[8];
    std::memcpy(copy, &record->Key()[8], 8);
    std::reverse(copy, copy + 8);

    uint64_t least_significant;
    std::memcpy(&least_significant, copy, sizeof(uint64_t));
    out << +least_significant;
  }
  out << ']';

  auto next = this->RightSibling();
  if (next) {
    out << ' ';
    next->Write(out);
  }
}

}
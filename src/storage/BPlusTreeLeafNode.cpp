#include <algorithm>
#include <cstring>

#include "Algorithm.h"
#include "BPlusTreeLeafNode.h"
#include "BPlusTreeInternalNode.h"
#include "KeyBearer.h"

namespace noid::storage {

static const K& GetKeyReference(BPlusTreeRecord& record) {
  return record.Key();
}

static bool LeftKeyIsLess(std::unique_ptr<BPlusTreeRecord>& lhs, std::unique_ptr<BPlusTreeRecord>& rhs) {
  return lhs->Key() < rhs->Key();
}

bool BPlusTreeLeafNode::IsMergeableWith(BPlusTreeLeafNode& sibling) {
  return this->records.size() + sibling.records.size() <= this->order * 2;
}

bool BPlusTreeLeafNode::CopyUp(const K &key) {
  auto must_increase_tree_height = this->IsRoot();
  if (must_increase_tree_height) {
    auto new_root = new BPlusTreeInternalNode(nullptr, this->order, key, this, this->next);
    this->parent = new_root;
  } else {
    this->parent->Insert(key, this, this->next);
  }

  return must_increase_tree_height;
}

bool BPlusTreeLeafNode::Redistribute() {
  if (this->next && this->Parent() == this->next->Parent() && this->next->IsRich()) {
    // Take the smallest record from our right sibling and append it to our records.
    auto taken_from_sibling = this->next->TakeSmallest();
    this->records.push_back(std::move(this->next->TakeSmallest()));

    // Replace key that points to us and sibling in parent entry.
    auto& next_smallest_from_sibling = this->next->SmallestKey();
    this->Parent()->GreatestNotExceeding(next_smallest_from_sibling)->Replace(next_smallest_from_sibling);

    return true;
  } else if (this->previous && this->Parent() == this->Previous()->Parent() && this->previous->IsRich()) {
    // Take the largest record from our left sibling and prepend it to our records.
    auto taken_from_sibling = this->Previous()->TakeLargest();
    this->records.insert(this->records.begin(), std::move(taken_from_sibling));

    // Replace key that points to us and sibling in parent entry.
    auto& smallest_key = this->SmallestKey();
    this->Parent()->GreatestNotExceeding(smallest_key)->Replace(smallest_key);

    return true;
  }

  return false;
}

TreeStructureChange BPlusTreeLeafNode::Merge() {
  BPlusTreeLeafNode* smallest = nullptr;
  BPlusTreeLeafNode* largest = nullptr;

  if (this->previous && this->previous->Parent() == this->parent && this->previous->IsMergeableWith(*this)) {
    smallest = this->previous;
    largest = this;
  } else if (this->next && this->next->Parent() == this->parent && this->next->IsMergeableWith(*this)) {
    smallest = this;
    largest = this->next;
  } else {
    // todo No mergeable sibling. Called out of order? Can we prove this never happens?
    return TreeStructureChange::None;
  }

  // After determining the smallest (smaller) and largest (larger) node, merging can take place. Always merge largest
  // into smallest, because this allows appending instead of prepending.

  // Since we only end up here if redistribution fails, we assume the node we will merge with is not rich,
  // and therefore a merge with this node and the sibling will by definition not lead to a full node, since this
  // node is poor.
  smallest->records.reserve(smallest->records.size() + largest->records.size());
  for (auto& record : largest->records) {
    smallest->records.push_back(std::move(record));
  }

  // Remove the greatest parent key which does not exceed the largest from the smallest node. This is the key
  // that points to both the smallest and largest node, whose records were just merged.
  auto left_largest_key = smallest->LargestKey();
  auto gne = smallest->parent->GreatestNotExceeding(left_largest_key);
  if (smallest->parent->Remove(gne->Key())) {

    // Let the next largest from the parent have its left_child point to smallest.
    auto next_largest_from_parent = smallest->parent->NextLargest(left_largest_key);
    if (next_largest_from_parent) {
      next_largest_from_parent->left_child = std::shared_ptr<BPlusTreeNode>(smallest);
    }
  }

  // Remove 'largest' from the linked list
  smallest->next = largest->next;
  if (smallest->next) { smallest->next->previous = smallest; }

  return smallest->parent->IsRoot() && smallest->parent->IsEmpty() ? TreeStructureChange::EmptyRoot : TreeStructureChange::None;
}

std::unique_ptr<BPlusTreeRecord> BPlusTreeLeafNode::TakeSmallest() {
  if (this->IsRich()) {
    auto smallest = std::move(this->records[0]);
    this->records.erase(this->records.begin());

    return std::move(smallest);
  }

  return {nullptr};
}

std::unique_ptr<BPlusTreeRecord> BPlusTreeLeafNode::TakeLargest() {
  if (this->IsRich()) {
    auto index = this->records.size() - 1;
    auto largest = std::move(this->records[index]);
    this->records.erase(this->records.begin() + static_cast<int64_t>(index));

    return std::move(largest);
  }

  return {nullptr};
}

BPlusTreeLeafNode::BPlusTreeLeafNode(BPlusTreeInternalNode* parent, uint8_t order, std::unique_ptr<BPlusTreeRecord> record)
: order(order), parent(parent), previous(nullptr), next(nullptr) {
  this->records.push_back(std::move(record));
}

bool BPlusTreeLeafNode::IsRoot() {
  return this->parent == nullptr;
}

bool BPlusTreeLeafNode::IsFull() {
  return this->records.size() > this->order * 2;
}

bool BPlusTreeLeafNode::IsPoor() {
  return this->IsRoot() ? this->records.empty() : this->records.size() < this->order;
}

bool BPlusTreeLeafNode::IsRich() {
  return this->IsRoot() ? this->records.size() > 1 : this->records.size() > this->order;
}

bool BPlusTreeLeafNode::Contains(const K &key) {
  return BinarySearch(this->records, 0, static_cast<int64_t>(this->records.size() - 1),
                      key,GetKeyReference) >= 0;
}

BPlusTreeInternalNode *BPlusTreeLeafNode::Parent() {
  return this->parent;
}

BPlusTreeLeafNode* BPlusTreeLeafNode::Previous() {
  return this->previous;
}

BPlusTreeLeafNode* BPlusTreeLeafNode::Next() {
  return this->next;
}

const K& BPlusTreeLeafNode::SmallestKey() {
  return this->records[0]->Key();
}

const K& BPlusTreeLeafNode::LargestKey() {
  return this->records[this->records.size() - 1]->Key();
}

void BPlusTreeLeafNode::SetParent(BPlusTreeInternalNode *p) {
  if (p) {
    this->parent = p;
  }
}

bool BPlusTreeLeafNode::Insert(const K &key, V &value) {
  auto index = noid::storage::BinarySearch(
      this->records, 0, static_cast<int64_t>(this->records.size() - 1), key,GetKeyReference);

  if (index == -1) {
    this->records.push_back(std::make_unique<BPlusTreeRecord>(key, value));
    std::sort(this->records.begin(), this->records.end(), LeftKeyIsLess);

    return true;
  } else {
    this->records[index]->Replace(value);
    return false;
  }
}

TreeStructureChange BPlusTreeLeafNode::Split() {
  if (this->records.size() < BTREE_MIN_ORDER) {
    return TreeStructureChange::None;
  }

  auto middle_index = this->records.size() / 2;

  // Create a new leaf and add the middle key to it.
  auto split = new BPlusTreeLeafNode(this->parent, this->order, std::move(this->records[middle_index]));

  // Add the largest half of the records to the new node
  for (auto i = middle_index + 1; i < this->records.size(); i++) {
    split->records.push_back(std::move(this->records[i]));
  }

  // Remove the slots of the records that were moved to the new node.
  this->records.resize(middle_index);

  // Put the leaf in position
  if (this->next) {
    this->next->previous = split;
  }

  split->previous = this;
  split->next = this->next;

  this->next = split;

  if (this->CopyUp(split->SmallestKey())) {
    return TreeStructureChange::NewRoot;
  }

  return TreeStructureChange::None;
}

std::optional<V> BPlusTreeLeafNode::Remove(const K &key) {
  if (this->records.empty()) {
    return std::nullopt;
  }

  auto index = BinarySearch(
      this->records, 0, static_cast<int64_t>(this->records.size() - 1), key,GetKeyReference);
  if (index >= 0) {
    auto record = std::move(this->records[index]);
    this->records.erase(this->records.begin() + index);

    return std::move(record)->Value();
  }

  return std::nullopt;
}

TreeStructureChange BPlusTreeLeafNode::Rearrange(const K &removed) {
  if (this->Redistribute()) {
    return TreeStructureChange::None;
  }

  return this->Merge();
}

void BPlusTreeLeafNode::Write(std::stringstream &out) {
  out << '[';
  for (auto i = 0; i < this->records.size(); i++) {
    auto record = this->records[i].get();

    if (i > 0) {
      out << ' ';
    }

    byte copy[8];
    std::memcpy(copy, &record->Key()[8], 8);
    std::reverse(copy, copy + 8);

    uint64_t least_significant;
    std::memcpy(&least_significant, copy, sizeof(uint64_t));
    out << +least_significant;
    out << '*';
  }
  out << ']';

  if (this->next) {
    out << ' ';
    this->next->Write(out);
  }
}

}

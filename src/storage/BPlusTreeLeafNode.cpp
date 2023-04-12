#include <algorithm>

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

SplitSideEffect BPlusTreeLeafNode::Split() {
  if (this->records.size() < BTREE_MIN_ORDER) {
    return SplitSideEffect::None;
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
    return SplitSideEffect::NewRoot;
  }

  return SplitSideEffect::None;
}

}

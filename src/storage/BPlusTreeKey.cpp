#include "BPlusTreeKey.h"

namespace noid::storage {

BPlusTreeKey::BPlusTreeKey(K key)
  : key(key), left_child(std::shared_ptr<BPlusTreeNode>()), right_child(std::shared_ptr<BPlusTreeNode>()) {}

const K &BPlusTreeKey::Key() const {
  return this->key;
}

bool operator==(const BPlusTreeKey& lhs, const BPlusTreeKey& rhs) {
  return lhs.Key() == rhs.Key();
}

bool operator<(const BPlusTreeKey &lhs, const BPlusTreeKey &rhs) {
  return lhs.Key() < rhs.Key();
}

}
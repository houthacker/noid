#include "BPlusTreeRecord.h"


namespace noid::storage {

bool operator==(BPlusTreeRecord &lhs, BPlusTreeRecord &rhs) {
  return lhs.Key() ==rhs.Key();
}

bool operator<(BPlusTreeRecord &lhs, BPlusTreeRecord &rhs) {
  return lhs.Key() < rhs.Key();
}

BPlusTreeRecord::BPlusTreeRecord(K key, V value) : key(key), value(std::move(value)) {}

const K &BPlusTreeRecord::Key() const {
  return this->key;
}

const V &BPlusTreeRecord::Value() {
  return this->value;
}

void BPlusTreeRecord::Replace(V &replacement) {
  this->value = std::move(replacement);
}

}
#include "BPlusTreeRecord.h"


namespace noid::algorithm {

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

const V &BPlusTreeRecord::Value() const& {
  return this->value;
}

V BPlusTreeRecord::Value() && {
  return std::move(this->value);
}

void BPlusTreeRecord::Replace(V &replacement) {
  this->value = std::move(replacement);
}

}
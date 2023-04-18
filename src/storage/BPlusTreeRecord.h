#ifndef NOID_SRC_STORAGE_BPLUSTREERECORD_H_
#define NOID_SRC_STORAGE_BPLUSTREERECORD_H_

#include "Shared.h"
#include "KeyBearer.h"

namespace noid::storage {

/**
 * @brief A container for a search key and the related data.
 */
class BPlusTreeRecord : public KeyBearer {
 private:
    K key;
    V value;

 public:

    /**
     * @brief Creates a new @c BPlusTreeRecord.
     * @details The @p key is copied, while the @p value is moved into this instance.
     *
     * @param key The search key.
     * @param value The related data.
     */
    BPlusTreeRecord(K key, V value);
    BPlusTreeRecord()= delete;
    BPlusTreeRecord(BPlusTreeRecord const&)= delete;
    BPlusTreeRecord(BPlusTreeRecord &&)= default;
    ~BPlusTreeRecord() override= default;

    BPlusTreeRecord& operator=(BPlusTreeRecord const&)= delete;
    BPlusTreeRecord& operator=(BPlusTreeRecord &&)= default;

    /**
     * @return A reference to the record key.
     */
    [[nodiscard]] const K& Key() const override;

    /**
     * @return A reference to the record value.
     */
    [[nodiscard]] V const& Value() const&;

    /**
     * @return Moves the value out of this record and returns it.
     */
    V Value() &&;

    /**
     * @brief Replaces the current value with the given one.
     * @details To avoid copying the data, the values' contents are moved instead of copied. This leaves
     * the given @p value in a valid, but hopefully useless state.
     *
     * @param replacement The replacement value.
     */
    void Replace(V& replacement);
};

  /**
   * @brief Tests the given @c BPlusTreeRecord instances on equality.
   *
   * @param lhs The left hand side.
   * @param rhs The right hand side.
   * @return Whether the given records are considered equal.
   */
  bool operator==(BPlusTreeRecord &lhs, BPlusTreeRecord &rhs);

  /**
   * @brief Tests whether @p lhs is considered less than @p rhs.
   *
   * @param lhs The left hand side.
   * @param rhs The right hand side.
   * @return Whether @p lhs is considered less than @p rhs.
   */
  bool operator<(BPlusTreeRecord &lhs, BPlusTreeRecord &rhs);

}

#endif //NOID_SRC_STORAGE_BPLUSTREERECORD_H_

#ifndef NOID_SRC_ALGORITHM_KEYBEARER_H_
#define NOID_SRC_ALGORITHM_KEYBEARER_H_

#include "Shared.h"

namespace noid::algorithm {

/**
 * @brief Interface for classes which expose a key.
 */
class KeyBearer {
 public:
    virtual ~KeyBearer() = default;

    /**
     * @return A reference to the key
     */
    [[nodiscard]] virtual const K& Key() const = 0;
};

}

#endif //NOID_SRC_ALGORITHM_KEYBEARER_H_

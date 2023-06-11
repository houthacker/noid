/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_PAGE_CONCEPTS_H
#define NOID_SRC_BACKEND_PAGE_CONCEPTS_H

#include <concepts>
#include <cstdint>
#include <memory>
#include <type_traits>

#include "backend/Types.h"
#include "backend/DynamicArray.h"

namespace noid::backend::page {

template<typename B, typename P>
concept PageBuilder = requires(B builder) {
  { builder.Build() } -> std::convertible_to<std::unique_ptr<const P>>;
};

template<typename P, typename B>
concept Page = PageBuilder<B, P> && requires(P, B, P page, DynamicArray<byte> && data) {
  { P::NewBuilder(std::move(data)) } -> std::same_as<std::unique_ptr<B>>;
  { page.ToBytes() } -> std::convertible_to<DynamicArray<byte>>;
};

}

#endif //NOID_SRC_BACKEND_PAGE_CONCEPTS_H

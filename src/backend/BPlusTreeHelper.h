/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_BPLUSTREEHELPER_H
#define NOID_BPLUSTREEHELPER_H

#ifndef NOID_INTERNAL_API
#error "Do not use Noid internal API directly. It is unsupported and subject to change or removal without notification."
#else

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "backend/Bits.h"
#include "backend/Pager.h"
#include "backend/Types.h"

#include "backend/concurrent/Concepts.h"
#include "backend/page/NodeRecord.h"
#include "backend/page/Overflow.h"

/**
 * @details All functionality implemented in this namespace is considered internal to Noid and therefore is subject
 * to change or removal without notice.
 *
 * @author houthacker
 */
namespace noid::backend::details {

/**
 * @brief Configures and returns a new @c NodeRecordBuilder.
 *
 * @details The builder is configures using the given parameters. On return, the @c NodeRecordBuilder requires no
 * additional configuration and @c NodeRecordBuilder::Build() can be called safely.
 * @warning This is a Noid internal API function and as such it is unsupported and subject to change or removal without notice.
 *
 * @param key The search key.
 * @param value The record value.
 * @param first_overflow_page The page number of the first value overflow page, or @c NULL_PAGE if no such page is required.
 * @return The new @c NodeRecordBuilder.
 */
std::shared_ptr<page::NodeRecordBuilder> CreateNodeRecordBuilder(SearchKey& key, const V& value,
    PageNumber first_overflow_page)
{
  auto record_builder = page::NodeRecord::NewBuilder();
  record_builder->WithSearchKey(key);

  if (first_overflow_page == NULL_PAGE) {
    record_builder->WithInlinePayload(
        read_container<byte, std::array<byte, page::NodeRecord::INLINE_PAYLOAD_SIZE>>(value, 0, value.size()),
        safe_cast<uint8_t>(value.size()));
  }
  else {
    record_builder->WithOverflowPayload(
        read_container<byte, std::array<byte, page::NodeRecord::OVERFLOW_PAYLOAD_SIZE>>(value, 0,
            page::NodeRecord::OVERFLOW_PAYLOAD_SIZE), first_overflow_page);
  }

  return record_builder;
}

/**
 * @brief Creates @c Overflow pages from @p value and writes them to storage.
 *
 * @details The overflow pages are written contiguously to pages [first, last_exclusive). If @p first is @c NULL_PAGE,
 * this method returns immediately.
 * @note This method assumes (and does not validate whether) @p first is smaller than @p last_exclusive.
 * @warning This is a Noid internal API function and as such it is unsupported and subject to change or removal without notice.
 *
 * @tparam Lockable A type with which a unique lock can be acquired on a resource.
 * @tparam SharedLockable A type with which a shared lock can be acquired on a resource.
 * @param value The value to be written.
 * @param page_range The range of pages first (inclusive), last (exclusive) to write the overflow to.
 * @param pager The @c Pager to use when writing the pages.
 */
template<concurrent::Lockable Lockable, concurrent::SharedLockable SharedLockable>
void WriteOverflow(const V& value, std::pair<PageNumber, PageNumber> page_range,
    std::shared_ptr<Pager<Lockable, SharedLockable>> pager)
{
  if (page_range.first == NULL_PAGE) {
    return;
  }

  decltype(value.size()) value_cursor = page::NodeRecord::OVERFLOW_PAYLOAD_SIZE;

  // Set up all pages and write them using the Pager.
  for (PageNumber current_page = page_range.first; current_page < page_range.second; current_page++) {
    std::shared_ptr<page::OverflowBuilder> overflow_builder = pager->template NewBuilder<page::Overflow,
                                                                                         page::OverflowBuilder>();

    // Always create a payload of maximum data size, since this keeps the page size on storage consistent.
    auto payload = DynamicArray<byte>(overflow_builder->MaxDataSize());

    // The last page to write might not be full, so calculate the amount of bytes to be written.
    auto next_page = current_page + 1;
    auto write_size = static_cast<uint16_t>(next_page < page_range.second ? payload.size() : value.size()
        - value_cursor);

    // Fill up the payload
    write_contiguous_container<byte>(payload, 0, value, value_cursor, write_size);

    // And create & write the overflow page
    auto overflow_page = overflow_builder->WithPayloadSize(write_size)->WithData(std::move(payload))->Build();
    pager->template WritePage<page::Overflow, page::OverflowBuilder>(*overflow_page, current_page);

    value_cursor += write_size;
  }
}

}

#endif

#endif //NOID_BPLUSTREEHELPER_H

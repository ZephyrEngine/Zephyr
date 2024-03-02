
#pragma once

#include <zephyr/integer.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/panic.hpp>
#include <atomic>

namespace zephyr {

  /**
   * Allocates and wraps a 64-bit ID that is unique during the execution time of the program.
   */
  class UID : public NonCopyable {
    public:
      /// @returns the 64-bit unique ID
      explicit operator u64() const {
        return m_uid;
      }

    private:
      static u64 AllocateUID() {
        const u64 uid = s_next_id.fetch_add(1u, std::memory_order_relaxed);

        if(uid == 0ull) [[unlikely]] {
          ZEPHYR_PANIC("UID: ran out of all IDs. What?");
        }

        return uid;
      }

      static std::atomic<u64> s_next_id; // initialized to one

      u64 m_uid = AllocateUID();
  };

} // namespace zephyr



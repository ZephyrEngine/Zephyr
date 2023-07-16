
#pragma once

#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>
#include <limits>

namespace zephyr {

  /**
   * This is the base class for all kinds of renderer resources, such as geometries,
   * geometry buffers or textures and provides a simple interface for querying the
   * current version of the resource, as well as bumping the resource version to
   * invalidate any caches.
   */
  class Resource : public NonCopyable, public NonMoveable {
    public:
      virtual ~Resource() = default;

      /// @returns the current version of the resource
      [[nodiscard]] u64 CurrentVersion() const {
        return m_version;
      }

      /// Mark the resource as dirty by bumping the version number.
      virtual void MarkAsDirty() {
        if(m_version == std::numeric_limits<u64>::max()) {
          // If this happens something is very messed up and m_version most likely was corrupted.
          ZEPHYR_PANIC("The 64-bit version counter overflowed. What?")
        }

        m_version++;
      }

    private:
      u64 m_version{}; ///< current 64-bit version number of the resource
  };

} // namespace zephyr

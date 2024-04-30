
#pragma once

#include <zephyr/integer.hpp>
#include <zephyr/float.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>
//#include <zephyr/uid.hpp>
#include <vector>

namespace zephyr {

  class Geometry : NonCopyable, NonMoveable {
    public:
//      /// @returns the Unique ID of this geometry
//      [[nodiscard]] u64 GetUID() const {
//        return (u64)m_uid;
//      }

      std::vector<u32> m_indices{};
      std::vector<f32> m_positions{};

//    private:
//      UID m_uid{}; //< Unique ID of this geometry
  };

} // namespace zephyr

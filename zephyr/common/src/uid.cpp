
#include <zephyr/uid.hpp>

namespace zephyr {

  std::atomic<u64> UID::s_next_id = 1ull;

} // namespace zephyr

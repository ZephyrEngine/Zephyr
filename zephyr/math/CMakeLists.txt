
set(HEADERS_PUBLIC
  include/zephyr/math/box3.hpp
  include/zephyr/math/frustum.hpp
  include/zephyr/math/matrix4.hpp
  include/zephyr/math/plane.hpp
  include/zephyr/math/quaternion.hpp
  include/zephyr/math/traits.hpp
  include/zephyr/math/vector.hpp
)

add_library(zephyr-math INTERFACE ${HEADERS_PUBLIC})

target_include_directories(zephyr-math INTERFACE include)
target_link_libraries(zephyr-math INTERFACE zephyr-common zephyr-cxx-opts)

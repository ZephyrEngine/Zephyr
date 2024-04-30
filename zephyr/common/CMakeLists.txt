
set(SOURCES
  src/panic.cpp
  src/uid.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/bit.hpp
  include/zephyr/float.hpp
  include/zephyr/integer.hpp
  include/zephyr/literal.hpp
  include/zephyr/meta.hpp
  include/zephyr/non_copyable.hpp
  include/zephyr/non_moveable.hpp
  include/zephyr/panic.hpp
  include/zephyr/punning.hpp
  include/zephyr/result.hpp
  include/zephyr/uid.hpp
  include/zephyr/vector_n.hpp
)

add_library(zephyr-common ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-common PUBLIC include)
target_link_libraries(zephyr-common PUBLIC fmt zephyr-cxx-opts)

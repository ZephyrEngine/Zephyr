
set(SOURCES
  src/panic.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/bit.hpp
  include/zephyr/float.hpp
  include/zephyr/integer.hpp
  include/zephyr/meta.hpp
  include/zephyr/panic.hpp
  include/zephyr/punning.hpp
  include/zephyr/vector_n.hpp
)

add_library(zephyr-common ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_compile_features(zephyr-common PUBLIC cxx_std_20)
target_include_directories(zephyr-common PUBLIC include)
target_link_libraries(zephyr-common PUBLIC fmt)

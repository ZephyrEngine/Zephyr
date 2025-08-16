
set(SOURCES
  src/gltf.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/zephyr/io/gltf.hpp
)

add_library(zephyr-io ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_link_libraries(zephyr-io PUBLIC fmt nlohmann_json stb zephyr-common zephyr-renderer zephyr-scene zephyr-cxx-opts)
target_include_directories(zephyr-io PUBLIC include PRIVATE src)

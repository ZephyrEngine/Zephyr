
set(SOURCES
  src/main.cpp
  src/main_window.cpp
)

set(HEADERS
  src/shader/triangle.frag.h
  src/shader/triangle.vert.h
  src/main_window.hpp
)

find_package(SDL2 REQUIRED)

add_executable(zephyr-next ${SOURCES} ${HEADERS})

target_link_libraries(zephyr-next PRIVATE zephyr zephyr-cxx-opts SDL2::SDL2)
target_include_directories(zephyr-next PRIVATE include)
target_include_directories(zephyr-next PRIVATE src)

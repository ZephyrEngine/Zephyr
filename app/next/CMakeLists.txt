
set(SOURCES
  src/main.cpp
  src/main_window.cpp
)

set(HEADERS
  src/main_window.hpp
)

find_package(SDL2 REQUIRED)

add_executable(zephyr-next ${SOURCES} ${HEADERS})

target_link_libraries(zephyr-next PRIVATE zephyr nlohmann_json)
target_include_directories(zephyr-next PRIVATE include)
target_include_directories(zephyr-next PRIVATE src)

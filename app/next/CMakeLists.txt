
set(SOURCES
  src/main.cpp
  src/main_window.cpp
)

set(HEADERS
  src/gltf_loader.hpp
  src/main_window.hpp
)

find_package(SDL2 REQUIRED)
add_subdirectory(../../external/mgpu/plugins/sdl2 ${CMAKE_CURRENT_BINARY_DIR}/external/mgpu-plugin-sdl2)

add_executable(zephyr-next ${SOURCES} ${HEADERS})

target_link_libraries(zephyr-next PRIVATE zephyr nlohmann_json stb mgpu-plugin-sdl2)
target_include_directories(zephyr-next PRIVATE include src)

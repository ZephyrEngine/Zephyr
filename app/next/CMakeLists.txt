
set(SOURCES
  src/main.cpp
  src/main_window.cpp
  src/sdl2_mgpu/sdl2_mgpu.hpp
)

set(HEADERS
  src/gltf_loader.hpp
  src/main_window.hpp
  src/sdl2_mgpu/sdl2_mgpu.cpp
)

find_package(SDL2 REQUIRED)

# TODO: clean this up
if(APPLE)
  enable_language(OBJCXX)
  list(APPEND SOURCES
    src/sdl2_mgpu/metal_layer_from_ns_window.mm
  )
endif()

add_executable(zephyr-next ${SOURCES} ${HEADERS})

target_link_libraries(zephyr-next PRIVATE zephyr nlohmann_json stb)
target_include_directories(zephyr-next PRIVATE include)
target_include_directories(zephyr-next PRIVATE src)

# TODO: clean this up
if(APPLE)
  target_link_libraries(zephyr-next PRIVATE
    "-framework Cocoa"
    "-framework QuartzCore"
  )
endif()

# TODO: clean this up
target_link_libraries(zephyr-next PRIVATE mgpu)
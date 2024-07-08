
# TODO: split this up into multiple libraries in the future (i.e. one core library and one library per backend)
# TODO: remove dependency on SDL2 if possible

set(SOURCES
  src/dummy.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_library(zephyr-gpu ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(zephyr-gpu PUBLIC include)
target_include_directories(zephyr-gpu PRIVATE src)
target_link_libraries(zephyr-gpu PUBLIC zephyr-common zephyr-cxx-opts SDL2::SDL2 Vulkan::Vulkan OpenGL::GL GLEW::GLEW)
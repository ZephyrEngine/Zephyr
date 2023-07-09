
add_subdirectory(fmt)

# TODO: VMA wants this. Maybe only add VMA if we build with the Vulkan backend?
find_package(Vulkan REQUIRED)
add_subdirectory(VulkanMemoryAllocator)

set(SKIP_GLSLANG_INSTALL ON CACHE INTERNAL "")
set(ENABLE_GLSLANG_BINARIES OFF CACHE INTERNAL "")
set(ENABLE_CTEST OFF CACHE INTERNAL "")
add_subdirectory(glslang)

add_library(stb stb.cpp stb/stb_image.h)
target_include_directories(stb PUBLIC stb)
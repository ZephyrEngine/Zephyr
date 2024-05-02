
add_subdirectory(fmt)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

# TODO: VMA wants this. Maybe only add VMA if we build with the Vulkan backend?
find_package(Vulkan REQUIRED)
add_subdirectory(VulkanMemoryAllocator)


add_subdirectory(EASTL)
add_subdirectory(fmt)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE INTERNAL "")
set(SPIRV_CROSS_CLI OFF CACHE INTERNAL "")
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE INTERNAL "")
add_subdirectory(SPIRV-Cross)

# TODO: VMA wants this. Maybe only add VMA if we build with the Vulkan backend?
find_package(Vulkan REQUIRED)
add_subdirectory(VulkanMemoryAllocator)

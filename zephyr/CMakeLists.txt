
add_subdirectory(common)
add_subdirectory(io)
add_subdirectory(logger)
add_subdirectory(math)
add_subdirectory(renderer)
add_subdirectory(scene)

add_library(zephyr INTERFACE)

target_link_libraries(zephyr INTERFACE zephyr-common zephyr-io zephyr-logger zephyr-math zephyr-renderer zephyr-scene)

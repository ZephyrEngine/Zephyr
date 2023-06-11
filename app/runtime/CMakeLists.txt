
set(SOURCES
  src/application.cpp
  src/main.cpp
)

set(HEADERS
  src/application.hpp
)

add_executable(zephyr-runtime ${SOURCES} ${HEADERS})

target_include_directories(zephyr-runtime PRIVATE src)
target_link_libraries(zephyr-runtime PRIVATE zephyr zephyr-cxx-opts)
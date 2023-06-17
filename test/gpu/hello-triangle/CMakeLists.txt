
set(SOURCES
  src/main.cpp
)

set(HEADERS
  src/shader/triangle.frag.h
  src/shader/triangle.vert.h
)

add_executable(hello-triangle ${SOURCES} ${HEADERS})
target_link_libraries(hello-triangle PRIVATE zephyr fmt)
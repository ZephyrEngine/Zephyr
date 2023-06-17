
set(SOURCES
  src/main.cpp
)

set(HEADERS
  src/shader/triangle.frag.h
  src/shader/triangle.vert.h
)

add_executable(hello-world ${SOURCES} ${HEADERS})
target_link_libraries(hello-world PRIVATE zephyr fmt)
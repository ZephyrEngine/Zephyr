
set(SOURCES
  src/main.cpp
)

set(HEADERS
  src/shader/mesh.frag.h
  src/shader/mesh.vert.h
)

add_executable(hello-cube ${SOURCES} ${HEADERS})
target_link_libraries(hello-cube PRIVATE zephyr fmt)
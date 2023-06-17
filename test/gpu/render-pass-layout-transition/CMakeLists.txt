
set(SOURCES
  src/main.cpp
)

set(HEADERS
  src/shader/mesh.frag.h
  src/shader/mesh.vert.h
)

add_executable(render-pass-layout-transition ${SOURCES} ${HEADERS})
target_link_libraries(render-pass-layout-transition PRIVATE zephyr fmt)
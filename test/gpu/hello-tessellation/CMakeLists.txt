
set(SOURCES
  src/main.cpp
)

set(HEADERS
  src/shader/mesh.frag.h
  src/shader/mesh.tese.h
  src/shader/mesh.tesc.h
  src/shader/mesh.vert.h
)

add_executable(hello-tessellation ${SOURCES} ${HEADERS})
target_link_libraries(hello-tessellation PRIVATE zephyr fmt)
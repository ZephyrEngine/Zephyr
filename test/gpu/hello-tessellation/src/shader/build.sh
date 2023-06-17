#!/bin/sh

glslangValidator -S vert -V100 --vn mesh_vert -o mesh.vert.h mesh.vert.glsl
glslangValidator -S frag -V100 --vn mesh_frag -o mesh.frag.h mesh.frag.glsl
glslangValidator -S tesc -V100 --vn mesh_tesc -o mesh.tesc.h mesh.tesc.glsl
glslangValidator -S tese -V100 --vn mesh_tese -o mesh.tese.h mesh.tese.glsl

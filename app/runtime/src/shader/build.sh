#!/bin/sh

glslangValidator -S vert -V100 --vn mesh_vert -o mesh.vert.h mesh.vert.glsl
glslangValidator -S frag -V100 --vn mesh_frag -o mesh.frag.h mesh.frag.glsl

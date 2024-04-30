#!/bin/sh

glslangValidator -S vert -V100 --vn triangle_vert -o triangle.vert.h triangle.vert.glsl
glslangValidator -S frag -V100 --vn triangle_frag -o triangle.frag.h triangle.frag.glsl

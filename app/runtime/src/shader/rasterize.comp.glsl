#version 450

// @todo: how to pick this ideally? and how exactly do we use workgroup-local memory?
layout (local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba32f) uniform writeonly highp image2D u_output_map;

void main() {
  if(gl_GlobalInvocationID.x < 128) {
    if(gl_GlobalInvocationID.y < 128) {
      imageStore(u_output_map, ivec2(gl_GlobalInvocationID.xy), vec4(1.0, 0.0, 0.0, 1.0));
    } else {
      imageStore(u_output_map, ivec2(gl_GlobalInvocationID.xy), vec4(0.0, 0.0, 1.0, 1.0));
    }
  } else {
    if(gl_GlobalInvocationID.y < 128) {
      imageStore(u_output_map, ivec2(gl_GlobalInvocationID.xy), vec4(0.0, 1.0, 0.0, 1.0));
    } else {
      imageStore(u_output_map, ivec2(gl_GlobalInvocationID.xy), vec4(1.0, 1.0, 1.0, 1.0));
    }
  }
}

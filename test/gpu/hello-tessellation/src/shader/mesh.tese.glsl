#version 450

layout(triangles, equal_spacing, cw) in;

layout(location = 0) in vec3 v_in_position[];
layout(location = 1) in vec3 v_in_color[];

layout(location = 0) out vec3 v_out_color;

layout(push_constant) uniform constants {
  mat4 projection;
  mat4 model_view;
} p_transform;

vec3 interpolate3D(vec3 a, vec3 b, vec3 c) {
  return a * gl_TessCoord.x + b * gl_TessCoord.y + c * gl_TessCoord.z;
}

void main() {
  v_out_color = interpolate3D(v_in_color[0], v_in_color[1], v_in_color[2]);

  vec3 position = interpolate3D(v_in_position[0], v_in_position[1], v_in_position[2]);

  position.y += sin(position.x * 5.0) * 0.1;

  gl_Position = p_transform.projection * vec4(position, 1.0);
}

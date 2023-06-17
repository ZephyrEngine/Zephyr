#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 v_color;

layout(push_constant) uniform constants {
  mat4 projection;
  mat4 model_view;
} p_transform;

void main() {
  v_color = a_color;

  gl_Position = p_transform.projection * p_transform.model_view * vec4(a_position, 1.0);
}

#version 450

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 0, std140) uniform Material {
  vec4 u_color;
};

layout(set = 0, binding = 1) uniform sampler2D u_texture;
layout(set = 0, binding = 2) uniform samplerCube u_cube_map;

void main() {
  frag_color = texture(u_texture, v_uv);
}

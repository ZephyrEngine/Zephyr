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
  vec3 color = v_color;

  // Cause some genuine GPU usage
  for(int i = 0; i < 1024; i++) {
    color += sin(color) * 0.000001;
  }

  frag_color = texture(u_texture, v_uv);
  if(v_uv.x > 0.5) {
    frag_color = texture(u_cube_map, vec3(v_uv, 0.5));
  }
  //frag_color = vec4(color * 0.5 + u_color.rgb * 0.5, 1.0);
}

#version 450

layout(location = 0) in vec3 v_color;

layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 0, std140) uniform Material {
  vec4 u_color;
};

void main() {
  vec3 color = v_color;

  // Cause some genuine GPU usage
  for(int i = 0; i < 1024; i++) {
    color += sin(color) * 0.000001;
  }

  frag_color = vec4(color * 0.5 + u_color.rgb * 0.5, 1.0);
}

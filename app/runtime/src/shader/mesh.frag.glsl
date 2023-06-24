#version 450

layout(location = 0) in vec3 v_color;

layout(location = 0) out vec4 frag_color;

void main() {
  vec3 color = v_color;

  // Cause some genuine GPU usage
  for(int i = 0; i < 1024; i++) {
    color += sin(color) * 0.000001;
  }

  frag_color = vec4(color, 1.0);
}

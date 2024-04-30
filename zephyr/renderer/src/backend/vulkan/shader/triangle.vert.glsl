#version 450

layout(location = 0) out vec3 v_color;
layout(location = 1) out vec2 v_uv;

layout(push_constant) uniform Asd {
  mat4 projection;
  mat4 modelview;
} u_transform;

void main() {
  switch(gl_VertexIndex) {
    case 0: {
      gl_Position = u_transform.projection * u_transform.modelview * vec4(-0.5,  0.5, 0.0, 1.0);
      v_color = vec3(1.0, 0.0, 0.0);
      v_uv = vec2(0.0, 0.0);
      break;
    }
    case 1: {
      gl_Position = u_transform.projection * u_transform.modelview * vec4( 0.5,  0.5, 0.0, 1.0);
      v_color = vec3(0.0, 1.0, 0.0);
      v_uv = vec2(1.0, 0.0);
      break;
    }
    case 2: {
      gl_Position = u_transform.projection * u_transform.modelview * vec4( 0.0, -0.5, 0.0, 1.0);
      v_color = vec3(0.0, 0.0, 1.0);
      v_uv = vec2(0.5, 1.0);
      break;
    }
  }
}
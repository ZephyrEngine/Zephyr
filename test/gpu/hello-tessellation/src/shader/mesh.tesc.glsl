#version 450

// define the number of control points in the output patch
layout(vertices = 3) out;

layout(location = 0) in vec3 v_in_position[];
layout(location = 1) in vec3 v_in_color[];

layout(location = 0) out vec3 v_out_position[];
layout(location = 1) out vec3 v_out_color[];

float get_tess_level(float dist0, float dist1) {
  float dist = 0.5 * (dist0 + dist1);

  return 32.0 - (min(dist, 20.0) / 20.0 * 28.0);
}

void main() {
  v_out_position[gl_InvocationID] = v_in_position[gl_InvocationID];
  v_out_color[gl_InvocationID] = v_in_color[gl_InvocationID];

  float dist0 = length(v_out_position[0]);
  float dist1 = length(v_out_position[1]);
  float dist2 = length(v_out_position[2]);

  gl_TessLevelOuter[0] = get_tess_level(dist1, dist2);
  gl_TessLevelOuter[1] = get_tess_level(dist2, dist0);
  gl_TessLevelOuter[2] = get_tess_level(dist0, dist1);
  gl_TessLevelInner[0] = gl_TessLevelOuter[2];
}
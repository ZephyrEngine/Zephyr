#version 450

// @todo: how to pick this ideally? and how exactly do we use workgroup-local memory?
layout (local_size_x = 16, local_size_y = 16) in;

struct VertexIn {
  vec4 position;
  vec4 color;
};

layout(set = 0, binding = 0, rgba32f) uniform writeonly highp image2D u_output_map;

layout(set = 0, binding = 1) buffer VertexData {
  VertexIn vertices[];
};

struct Vertex {
  vec2 position;
  vec3 color;
};

vec3 get_barycentric_coordinates(vec2 v1, vec2 v2, vec2 v3, vec2 point) {
  // @todo: optimize the math
  float det = (v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y);
  float x = ((v2.y - v3.y) * (point.x - v3.x) + (v3.x - v2.x) * (point.y - v3.y)) / det;
  float y = ((v3.y - v1.y) * (point.x - v3.x) + (v1.x - v3.x) * (point.y - v3.y)) / det;
  float z = 1.0 - x - y;

  return vec3(x, y, z);
}

void main() {
  vec2 ndc = vec2(gl_GlobalInvocationID.xy) / vec2(imageSize(u_output_map)) * 2.0 - 1.0;

  vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

  // @todo: Make this more flexible:
  const int triangle_count = 2;

  for(int i = 0; i < triangle_count; i++) {
    int base = i * 3;

    Vertex v0 = Vertex(vertices[base + 0].position.xy, vertices[base + 0].color.rgb);
    Vertex v1 = Vertex(vertices[base + 1].position.xy, vertices[base + 1].color.rgb);
    Vertex v2 = Vertex(vertices[base + 2].position.xy, vertices[base + 2].color.rgb);

    vec3 bary = get_barycentric_coordinates(v0.position, v1.position, v2.position, ndc);

    // @todo: for some reason an epsilon is needed... why?
    if (bary.x >= 0.0 && bary.y >= 0.0 && bary.z >= 0.0 && bary.x + bary.y + bary.z <= 1.0000001) {
      color.rgb = v0.color * bary.x + v1.color * bary.y + v2.color * bary.z;
    }
  }

  imageStore(u_output_map, ivec2(gl_GlobalInvocationID.xy), color);
}
#version 450

// @todo: how to pick this ideally? and how exactly do we use workgroup-local memory?
layout (local_size_x = 16, local_size_y = 16) in;

struct VertexIn {
  vec4 position;
  vec4 color;
};

layout(set = 0, binding = 0, rgba32f) uniform writeonly highp image2D u_output_map;

layout(set = 0, binding = 1) buffer VertexBuffer {
  VertexIn a_vertices[];
};

layout(set = 0, binding = 2) buffer IndexBuffer {
  uint a_indices[];
};

layout(set = 0, binding = 3, std140) uniform UniformBuffer {
  mat4 u_transform;
};

struct Vertex {
  vec3 position;
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
  float current_depth = 1.0;

  // @todo: Make this more flexible:
  const int triangle_count = 12;

  for(int i = 0; i < triangle_count; i++) {
    int base = i * 3;

    Vertex v[3];

    for(int j = 0; j < 3; j++) {
      VertexIn vertex_in = a_vertices[a_indices[base + j]];

      vec4 position = u_transform * vertex_in.position;
      vec3 clip = position.xyz / position.w;

      v[j] = Vertex(clip, vertex_in.color.rgb);
    }

    // @todo: do vertex clipping (maybe just do it per-pixel?)

    vec3 bary = get_barycentric_coordinates(v[0].position.xy, v[1].position.xy, v[2].position.xy, ndc);

    if (bary.x >= 0.0 && bary.y >= 0.0 && bary.z >= 0.0) {
      float depth = v[0].position.z * bary.x + v[1].position.z * bary.y + v[2].position.z * bary.z;

      if(depth <= current_depth) {
        current_depth = depth;
        color.rgb = v[0].color * bary.x + v[1].color * bary.y + v[2].color * bary.z;
      }
    }
  }

  imageStore(u_output_map, ivec2(gl_GlobalInvocationID.xy), color);
}

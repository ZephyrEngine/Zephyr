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

layout(push_constant) uniform constants {
  int u_triangle_count;
};

struct Vertex {
  vec3 position;
  vec3 color;
};

struct CulledTriangleList {
  int count;
  int list[16384];
};

shared CulledTriangleList s_culled_triangle_list;

vec3 get_barycentric_coordinates(vec2 v1, vec2 v2, vec2 v3, vec2 point) {
  // @todo: optimize the math
  float det = (v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y);
  float x = ((v2.y - v3.y) * (point.x - v3.x) + (v3.x - v2.x) * (point.y - v3.y)) / det;
  float y = ((v3.y - v1.y) * (point.x - v3.x) + (v1.x - v3.x) * (point.y - v3.y)) / det;
  float z = 1.0 - x - y;

  return vec3(x, y, z);
}

void main() {
  vec2 output_size = vec2(imageSize(u_output_map));

  // ################################################################# //
  // # Culling                                                       # //
  // ################################################################# //

  if(gl_LocalInvocationIndex == 0u) {
    s_culled_triangle_list.count = 0;
  }
  barrier();

  // @todo: confirm that this is always correct
  // @note: we shift by 8 because we have 256 threads per work/thread group
  int triangle0 = (u_triangle_count * int(gl_LocalInvocationIndex)) >> 8;
  int triangle1 = (u_triangle_count * (int(gl_LocalInvocationIndex) + 1)) >> 8;

  // @todo: optimize this
  vec2 tile_aabb_min = vec2(gl_WorkGroupID.xy * gl_WorkGroupSize.xy) / output_size;
  vec2 tile_aabb_max = tile_aabb_min + vec2(gl_WorkGroupSize) / output_size;
  tile_aabb_min = tile_aabb_min * 2.0 - 1.0;
  tile_aabb_max = tile_aabb_max * 2.0 - 1.0;

  for(int i = triangle0; i < triangle1; i++) {
    bool inside = false;

    vec2 tri_aabb_min = vec2(1.0, 1.0);
    vec2 tri_aabb_max = vec2(-1.0, -1.0);

    for(int j = 0; j < 3; j++) {
      VertexIn vertex_in = a_vertices[a_indices[i * 3 + j]];

      // @todo: projection multiple times is *not cool*
      vec4 position = u_transform * vertex_in.position;
      vec2 clip = position.xy / position.w;

      tri_aabb_min = min(tri_aabb_min, clip);
      tri_aabb_max = max(tri_aabb_max, clip);
    }

    // @todo: ensure that this really always is correct
    bool outside = tile_aabb_max.x < tri_aabb_min.x ||
                   tile_aabb_max.y < tri_aabb_min.y ||
                   tile_aabb_min.x > tri_aabb_max.x ||
                   tile_aabb_min.y > tri_aabb_max.y;

    if(!outside) {
      int list_index = atomicAdd(s_culled_triangle_list.count, 1);

      if(list_index < 16384) {
        s_culled_triangle_list.list[list_index] = i;
      }
    }
  }

  barrier();

  // ################################################################# //
  // # Rasterization                                                 # //
  // ################################################################# //

  vec2 ndc = vec2(gl_GlobalInvocationID.xy) / output_size * 2.0 - 1.0;

  vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
  float current_depth = 1.0;

  int triangle_count = s_culled_triangle_list.count;

  for(int i = 0; i < triangle_count; i++) {
    int base = s_culled_triangle_list.list[i] * 3;

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

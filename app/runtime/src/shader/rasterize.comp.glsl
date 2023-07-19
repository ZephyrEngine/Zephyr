#version 450

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
  mat4 u_view;
  mat4 u_projection;
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
  int list[8192];
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

#define pixels 4

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
  tile_aabb_min = tile_aabb_min * pixels * 2.0 - 1.0;
  tile_aabb_max = tile_aabb_max * pixels * 2.0 - 1.0;

  float max_passing_depth = 10000.0; // @todo: reduce once clipping has been implemented

  for(int i = triangle0; i < triangle1; i++) {
    bool inside = false;

    vec2 tri_aabb_min = vec2(1.0, 1.0);
    vec2 tri_aabb_max = vec2(-1.0, -1.0);

    vec4 view_position[3];
    vec3 ndc_position[3];

    for(int j = 0; j < 3; j++) {
      VertexIn vertex_in = a_vertices[a_indices[i * 3 + j]];

      view_position[j] = u_view * vertex_in.position;

      // @todo: projection multiple times is *not cool*
      vec4 position = u_projection * view_position[j];
      ndc_position[j] = position.xyz / position.w;

      tri_aabb_min = min(tri_aabb_min, ndc_position[j].xy);
      tri_aabb_max = max(tri_aabb_max, ndc_position[j].xy);
    }

    float facedness = dot(cross(view_position[1].xyz - view_position[0].xyz, view_position[2].xyz - view_position[0].xyz), view_position[0].xyz);

    float min_depth = min(ndc_position[0].z, min(ndc_position[1].z, ndc_position[2].z));

    // @todo: ensure that this really always is correct
    bool outside = tile_aabb_max.x < tri_aabb_min.x ||
                   tile_aabb_max.y < tri_aabb_min.y ||
                   tile_aabb_min.x > tri_aabb_max.x ||
                   tile_aabb_min.y > tri_aabb_max.y ||
                   facedness < 0.0;// ||
                   min_depth > max_passing_depth;

    if(!outside) {
      int list_index = atomicAdd(s_culled_triangle_list.count, 1);

      if(list_index < 8192) {
        s_culled_triangle_list.list[list_index] = i;
      }

      // Heuristic: if tri AABB area is larger than the tile AABB area,
      // then attempt to update our "farthest away possible depth value that can pass":
      float tri_aabb_area = (tri_aabb_max.x - tri_aabb_min.x) * (tri_aabb_max.y - tri_aabb_min.y);
      float tile_aabb_area = (tile_aabb_max.x - tile_aabb_min.x) * (tile_aabb_max.y - tile_aabb_min.y); // @todo: this can be a constant really

      if(tri_aabb_area > tile_aabb_area) {
        // @todo: some recomputation can probably be saved here:
        vec3 bary0 = get_barycentric_coordinates(ndc_position[0].xy, ndc_position[1].xy, ndc_position[2].xy, vec2(tile_aabb_min.x, tile_aabb_min.y));
        vec3 bary1 = get_barycentric_coordinates(ndc_position[0].xy, ndc_position[1].xy, ndc_position[2].xy, vec2(tile_aabb_max.x, tile_aabb_min.y));
        vec3 bary2 = get_barycentric_coordinates(ndc_position[0].xy, ndc_position[1].xy, ndc_position[2].xy, vec2(tile_aabb_min.x, tile_aabb_max.y));
        vec3 bary3 = get_barycentric_coordinates(ndc_position[0].xy, ndc_position[1].xy, ndc_position[2].xy, vec2(tile_aabb_max.x, tile_aabb_max.y));

        // @todo: can this be optimized somehow?
        if(
          bary0.x >= 0.0 && bary0.y >= 0.0 && bary0.z >= 0.0 &&
          bary1.x >= 0.0 && bary1.y >= 0.0 && bary1.z >= 0.0 &&
          bary2.x >= 0.0 && bary2.y >= 0.0 && bary2.z >= 0.0 &&
          bary3.x >= 0.0 && bary3.y >= 0.0 && bary3.z >= 0.0
        ) {
          float depth0 = bary0.x * ndc_position[0].z + bary0.y * ndc_position[1].z + bary0.z * ndc_position[2].z;
          float depth1 = bary1.x * ndc_position[0].z + bary1.y * ndc_position[1].z + bary1.z * ndc_position[2].z;
          float depth2 = bary2.x * ndc_position[0].z + bary2.y * ndc_position[1].z + bary2.z * ndc_position[2].z;
          float depth3 = bary3.x * ndc_position[0].z + bary3.y * ndc_position[1].z + bary3.z * ndc_position[2].z;

          float max_depth = max(depth0, max(depth1, max(depth2, depth3)));

          if(max_depth < max_passing_depth) {
            max_passing_depth = max_depth;
          }
        }
      }
    }
  }

  barrier();

  // ################################################################# //
  // # Rasterization                                                 # //
  // ################################################################# //

  int triangle_count = s_culled_triangle_list.count;

  float current_depth[pixels][pixels];

  for(int x = 0; x < pixels; x++) {
    for(int y = 0; y < pixels; y++) {
      uvec2 pixel_position = gl_GlobalInvocationID.xy * pixels + uvec2(x, y);

      imageStore(u_output_map, ivec2(pixel_position), vec4(1.0));

      current_depth[x][y] = 1.0;
    }
  }

  for(int i = 0; i < triangle_count; i++) {
    int base = s_culled_triangle_list.list[i] * 3;

    Vertex v[3];

    for(int j = 0; j < 3; j++) {
      VertexIn vertex_in = a_vertices[a_indices[base + j]];

      vec4 position = u_projection * u_view * vertex_in.position;
      vec3 clip = position.xyz / position.w;

      v[j] = Vertex(clip, vertex_in.color.rgb);
    }

    float min_depth = min(v[0].position.z, min(v[1].position.z, v[2].position.z));

//    if(min_depth > max_passing_depth) {
//      continue;
//    }

    for(int x = 0; x < pixels; x++) {
      for(int y = 0; y < pixels; y++) {
        uvec2 pixel_position = gl_GlobalInvocationID.xy * pixels + uvec2(x, y);
        vec2 ndc = vec2(pixel_position) / output_size * 2.0 - 1.0;

        vec3 bary = get_barycentric_coordinates(v[0].position.xy, v[1].position.xy, v[2].position.xy, ndc);

        if(bary.x >= 0.0 && bary.y >= 0.0 && bary.z >= 0.0) {
          float depth = v[0].position.z * bary.x + v[1].position.z * bary.y + v[2].position.z * bary.z;

          if(depth <= current_depth[x][y]) {
            current_depth[x][y] = depth;

            vec3 color = v[0].color * bary.x + v[1].color * bary.y + v[2].color * bary.z;

            imageStore(u_output_map, ivec2(pixel_position), vec4(color, 1.0));
          }
        }
      }
    }
  }
}

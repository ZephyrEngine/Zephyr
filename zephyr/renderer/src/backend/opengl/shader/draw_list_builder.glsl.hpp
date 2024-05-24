
#pragma once

namespace zephyr {

  static constexpr auto k_draw_list_builder_comp_glsl = R"(
    #version 460 core

    layout(local_size_x = 32) in;

    struct DrawCommand {
      uint data[5];
    };

    struct DrawCommandWithRenderBundleItemID {
      DrawCommand command;
      uint render_bundle_item_id;
    };

    struct RenderGeometryRenderData {
      // TODO(fleroviux): evaluate whether the packing can be tighter or not.
      vec4 aabb_min;
      vec4 aabb_max;
      DrawCommand draw_command;
    };

    struct RenderBundleItem {
      mat4 local_to_world;
      uint geometry_id;
    };

    layout(std430, binding = 0) readonly buffer RenderBundleBuffer {
      RenderBundleItem rb_render_bundle_items[];
    };

    layout(std430, binding = 1) buffer CommandBuffer {
      DrawCommandWithRenderBundleItemID b_command_buffer[];
    };

    layout(std430, binding = 2) readonly buffer GeometryBuffer {
      RenderGeometryRenderData rb_render_geometry_render_data[];
    };

    layout(std140, binding = 0) uniform Camera {
      mat4 u_projection;
      mat4 u_view;
      vec4 u_frustum_planes[6];
    };

    layout(std140, binding = 1) uniform DrawCount {
      uint u_draw_count;
    };

    layout(binding = 0) uniform atomic_uint u_draw_count_out;

    void main() {
      const uint render_bundle_item_id = gl_GlobalInvocationID.x;

      if(render_bundle_item_id == 0u) {
        atomicCounterExchange(u_draw_count_out, 0u);
      }
      barrier();

      if(render_bundle_item_id < u_draw_count) {
        RenderBundleItem render_bundle_item = rb_render_bundle_items[render_bundle_item_id];
        RenderGeometryRenderData render_data = rb_render_geometry_render_data[render_bundle_item.geometry_id];

        mat4 mv = u_view * render_bundle_item.local_to_world;

        // Model-Space Axis-Aligned Bounding Box
        vec4 model_aabb_min = render_data.aabb_min;
        vec4 model_aabb_max = render_data.aabb_max;

        // Camera-Space Axis-Aligned Bounding Box
        vec4 mv_min_x = mv[0] * model_aabb_min.x;
        vec4 mv_min_y = mv[1] * model_aabb_min.y;
        vec4 mv_min_z = mv[2] * model_aabb_min.z;
        vec4 mv_max_x = mv[0] * model_aabb_max.x;
        vec4 mv_max_y = mv[1] * model_aabb_max.y;
        vec4 mv_max_z = mv[2] * model_aabb_max.z;
        vec4 view_aabb_min = min(mv_min_x, mv_max_x) + min(mv_min_y, mv_max_y) + min(mv_min_z, mv_max_z) + mv[3];
        vec4 view_aabb_max = max(mv_min_x, mv_max_x) + max(mv_min_y, mv_max_y) + max(mv_min_z, mv_max_z) + mv[3];

        bool inside_frustum = true;

        for(int i = 0; i < 6; i++) {
          vec4 frustum_plane = u_frustum_planes[i];

          vec4 aabb_corner = vec4(
            frustum_plane.x > 0 ? view_aabb_max.x : view_aabb_min.x,
            frustum_plane.y > 0 ? view_aabb_max.y : view_aabb_min.y,
            frustum_plane.z > 0 ? view_aabb_max.z : view_aabb_min.z,
            -1.0
          );

          inside_frustum = inside_frustum && dot(aabb_corner, frustum_plane) >= 0.0;
        }

        if(inside_frustum) {
          uint draw_id = atomicCounterIncrement(u_draw_count_out);
          b_command_buffer[draw_id] = DrawCommandWithRenderBundleItemID(render_data.draw_command, render_bundle_item_id);
        }
      }
    }
  )";

} // namespace zephyr

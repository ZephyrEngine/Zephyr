
#pragma once

namespace zephyr {

static constexpr auto k_draw_call_vert_glsl = R"(
  #version 460 core

  struct RenderBundleItem {
    mat4 local_to_world;
    uint geometry_id;
    uint material_id;
  };

  struct DrawCommandWithRenderBundleItemID {
    uint command[5];
    uint render_bundle_item_id;
  };

  layout(std430, binding = 0) readonly buffer RenderBundleBuffer {
    RenderBundleItem rb_render_bundle_items[];
  };

  layout(std430, binding = 1) readonly buffer CommandBuffer {
    DrawCommandWithRenderBundleItemID rb_command_buffer[];
  };

  layout(std140, binding = 0) uniform Camera {
    mat4 u_projection;
    mat4 u_view;
  };

  layout(location = 0) in vec3 a_position;
  layout(location = 1) in vec3 a_normal;
  layout(location = 2) in vec2 a_uv;
  layout(location = 3) in vec3 a_color;

  flat out uint fv_material_id;
  out vec3 v_normal;
  out vec3 v_color;

  void main() {
    uint render_bundle_item_id = rb_command_buffer[gl_DrawID].render_bundle_item_id;

    fv_material_id = rb_render_bundle_items[render_bundle_item_id].material_id;
    v_normal = a_normal;
    v_color = a_color;
    gl_Position = u_projection * u_view * rb_render_bundle_items[render_bundle_item_id].local_to_world * vec4(a_position, 1.0);
  }
)";

static constexpr auto k_draw_call_frag_glsl = R"(
  #version 460 core

  layout(std430, binding = 2) readonly buffer MaterialBuffer {
    vec4 rb_material_color[];
  };

  layout(location = 0) out vec4 f_frag_color;

  flat in uint fv_material_id;
  in vec3 v_normal;
  in vec3 v_color;

  void main() {
    vec4 material_color = rb_material_color[fv_material_id];
    f_frag_color = vec4(v_normal * 0.25 + 0.25 + material_color.rgb * 0.5, 1.0);
  }
)";

} // namespace zephyr

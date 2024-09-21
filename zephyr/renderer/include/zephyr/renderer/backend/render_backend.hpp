
#pragma once

#include <zephyr/math/box3.hpp>
#include <zephyr/math/frustum.hpp>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/float.hpp>
#include <zephyr/hash.hpp>
#include <zephyr/integer.hpp>
#include <EASTL/hash_map.h>
#include <span>

namespace zephyr {

enum class RenderGeometryAttribute {
  Position,
  Normal,
  UV,
  Color,
  Count
};

struct RenderGeometryLayout {
  static_assert((int)RenderGeometryAttribute::Count <= 32);

  RenderGeometryLayout() = default;
  explicit RenderGeometryLayout(u32 key) : key{key} {}

  void AddAttribute(RenderGeometryAttribute attribute) {
    key |= 1ul << (int)attribute;
  }

  [[nodiscard]] bool HasAttribute(RenderGeometryAttribute attribute) const {
    return key & (1ul << (int)attribute);
  }

  u32 key = 0u;
};

// @todo: is this really the best way to represent the handle?
class RenderGeometry {
  public:
    virtual ~RenderGeometry() = default;

    [[nodiscard]] virtual RenderGeometryLayout GetLayout() const = 0;
    [[nodiscard]] virtual size_t GetGeometryID() const = 0;
    [[nodiscard]] virtual size_t GetNumberOfVertices() const = 0;
    [[nodiscard]] virtual size_t GetNumberOfIndices() const = 0;
};

struct RenderCamera {
  /**
   * This struct may be uploaded to a GPU buffer directly and therefore must match the following std430/std140 data layout:
   *   mat4 projection;
   *   mat4 view;
   *   vec4 frustum_planes[6];
   */
  Matrix4 projection{};
  Matrix4 view{};
  Frustum frustum{};
};

class RenderBackend {
  public:
    struct RenderBundleKey {
      bool uses_ibo;
      u32 geometry_layout;

      [[nodiscard]] bool operator==(const RenderBundleKey& other) const {
        return uses_ibo == other.uses_ibo && geometry_layout == other.geometry_layout;
      }
    };

    struct RenderBundleItem {
      RenderBundleItem(const Matrix4& local_to_world, u32 draw_command_id, u32 material_id, u64 entity_id) : local_to_world{local_to_world}, draw_command_id{draw_command_id}, material_id{material_id}, entity_id{entity_id} {}
      Matrix4 local_to_world;
      u32 draw_command_id;
      u32 material_id;
      u64 entity_id;
    };

    virtual ~RenderBackend() = default;

    /// Needs to be called from the render thread before performing any render operations.
    virtual void InitializeContext() = 0;

    /// Needs to be called from the render thread before destroying the render backend.
    virtual void DestroyContext() = 0;

    virtual RenderGeometry* CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) = 0;
    virtual void UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) = 0;
    virtual void UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) = 0;
    virtual void UpdateRenderGeometryAABB(RenderGeometry* render_geometry, const Box3& aabb) = 0;
    virtual void DestroyRenderGeometry(RenderGeometry* render_geometry) = 0;

    /// Just a quick thing for testing the rendering.
    virtual void Render(const RenderCamera& render_camera, const eastl::hash_map<RenderBundleKey, std::vector<RenderBundleItem>>& render_bundles) = 0;

    /// Start rendering the next frame.
    virtual void SwapBuffers() = 0;
};

} // namespace zephyr

template<>
struct eastl::hash<zephyr::RenderBackend::RenderBundleKey> {
  [[nodiscard]] std::size_t operator()(const zephyr::RenderBackend::RenderBundleKey& key) const noexcept {
    size_t h = 0;
    zephyr::hash_combine(h, key.uses_ibo);
    zephyr::hash_combine(h, key.geometry_layout);
    return h;
  }
};

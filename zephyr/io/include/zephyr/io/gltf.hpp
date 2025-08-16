
#pragma once

#include <zephyr/renderer/resource/geometry.hpp>
#include <zephyr/renderer/resource/material.hpp>
#include <zephyr/scene/scene_node.hpp>
#include <nlohmann/json.hpp>
#include <vector>

namespace zephyr {

// TODO(Gloria Goertz): Restructure this loader classlessly™️
class GLTFLoader {
  public:
    std::shared_ptr<SceneNode> Parse(const std::filesystem::path& path);

  private:
    using Buffer = std::vector<u8>;

    enum class ComponentType {
      Byte = 5120,
      UnsignedByte = 5121,
      Short = 5122,
      UnsignedShort = 5123,
      UnsignedInt = 5125,
      Float = 5126
    };

    enum class AccessorType {
      Scalar,
      Vec2,
      Vec3,
      Vec4,
      Mat2,
      Mat3,
      Mat4
    };

    struct BufferView {
      size_t buffer;
      size_t byte_offset;
      size_t byte_length;
      size_t byte_stride;
    };

    struct Accessor {
      size_t buffer_view;
      size_t byte_offset;
      ComponentType component_type;
      bool normalized;
      size_t count;
      AccessorType type;
    };

    struct Mesh {
      struct Primitive {
        std::shared_ptr<Geometry> geometry;
        std::shared_ptr<Material> material;
      };
      std::vector<Primitive> primitives;
    };

    void LoadBuffers(const nlohmann::json& gltf_json);
    void LoadBufferViews(const nlohmann::json& gltf_json);
    void LoadAccessors(const nlohmann::json& gltf_json);
    void LoadImages(const nlohmann::json& gltf_json);
    void LoadTextures(const nlohmann::json& gltf_json);
    void LoadMaterials(const nlohmann::json& gltf_json);
    void LoadMeshes(const nlohmann::json& gltf_json);
    void LoadVec3Accessor(const Accessor& accessor, Geometry::Accessor<Vector3> geometry_accessor);
    void LoadIndexBufferAccessor(const Accessor& accessor, std::span<u32> geometry_index_buffer);
    std::shared_ptr<SceneNode> LoadNodeHierarchy(const nlohmann::json& gltf_json, size_t node_index);

    std::filesystem::path m_base_path{};
    std::vector<Buffer> m_buffers{};
    std::vector<BufferView> m_buffer_views{};
    std::vector<Accessor> m_accessors{};
    std::vector<Mesh> m_meshes{};
    std::vector<std::shared_ptr<Texture2D>> m_images{};
    std::vector<std::shared_ptr<Texture2D>> m_textures{};
    std::vector<std::shared_ptr<Material>> m_materials{};
};

} // namespace zephyr

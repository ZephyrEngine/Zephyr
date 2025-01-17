
#pragma once

#include <zephyr/renderer/component/mesh.hpp>
#include <zephyr/renderer/resource/geometry.hpp>
#include <zephyr/renderer/resource/material.hpp>
#include <zephyr/renderer/resource/texture_2d.hpp>
#include <zephyr/scene/scene_node.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <stb_image.h>
#include <vector>

namespace zephyr {

class GLTFLoader {
  public:
    std::shared_ptr<SceneNode> Parse(const std::filesystem::path& path) {
      std::ifstream gltf_file{path};
      if(!gltf_file.good()) {
        ZEPHYR_PANIC("Failed to open GLTF file: {}", path.string());
      }

      // TODO(fleroviux): handle any potential errors from this
      const nlohmann::json gltf_json = nlohmann::json::parse(gltf_file);

      m_base_path = std::filesystem::path{path}.remove_filename();
      m_buffers.clear();
      m_buffer_views.clear();
      m_accessors.clear();
      m_meshes.clear();

      LoadBuffers(gltf_json);
      LoadBufferViews(gltf_json);
      LoadAccessors(gltf_json);
      LoadImages(gltf_json);
      LoadTextures(gltf_json);
      LoadMaterials(gltf_json);
      LoadMeshes(gltf_json);

      // TODO: proper scene parsing
      return LoadNodeHierarchy(gltf_json, 0u);
    }

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

    void LoadBuffers(const nlohmann::json& gltf_json) {
      if(!gltf_json.contains("buffers")) {
        return;
      }

      for(const nlohmann::json& buffer : gltf_json["buffers"]) {
        // TODO(fleroviux): handle URI-less buffers (used by GLBs?)
        if(!buffer.contains("uri")) {
          ZEPHYR_PANIC("Unhandled GLTF buffer without URI");
        }

        if(!buffer.contains("byteLength")) {
          ZEPHYR_PANIC("Missing byteLength property");
        }

        // TODO(fleroviux): handle IRIs (buffer from data URL)
        const std::filesystem::path uri = m_base_path / buffer["uri"].get<std::string>();
        const size_t byte_length = buffer["byteLength"].get<size_t>();

        std::ifstream buffer_file{uri, std::ios::binary};
        if(!buffer_file.good()) {
          ZEPHYR_PANIC("Failed to load buffer: {}", uri.string());
        }

        std::vector<u8> buffer_data{};
        buffer_data.assign(std::istreambuf_iterator<char>{buffer_file}, std::istreambuf_iterator<char>{});
        if(buffer_data.size() < byte_length) {
          ZEPHYR_PANIC("Buffer file is smaller than the specified size of the buffer. {} {}", buffer_data.size(), byte_length);
        }

        m_buffers.push_back(std::move(buffer_data));
      }
    }

    void LoadBufferViews(const nlohmann::json& gltf_json) {
      if(!gltf_json.contains("bufferViews")) {
        return;
      }

      for(const nlohmann::json& buffer_view : gltf_json["bufferViews"]) {
        BufferView parsed_buffer_view{};

        if(buffer_view.contains("buffer")) {
          parsed_buffer_view.buffer = buffer_view["buffer"].get<size_t>();
        } else {
          ZEPHYR_PANIC("Attribute 'buffer' missing from buffer view");
        }

        if(buffer_view.contains("byteOffset")) {
          parsed_buffer_view.byte_offset = buffer_view["byteOffset"].get<size_t>();
        } else {
          parsed_buffer_view.byte_offset = 0u;
        }

        if(buffer_view.contains("byteLength")) {
          parsed_buffer_view.byte_length = buffer_view["byteLength"].get<size_t>();
        } else {
          ZEPHYR_PANIC("Attribute 'byteLength' missing from buffer view");
        }

        if(buffer_view.contains("byteStride")) {
          parsed_buffer_view.byte_stride = buffer_view["byteStride"].get<size_t>();
        } else {
          parsed_buffer_view.byte_stride = 0u;
        }

        m_buffer_views.push_back(parsed_buffer_view);
      }
    }

    void LoadAccessors(const nlohmann::json& gltf_json) {
      if(!gltf_json.contains("accessors")) {
        return;
      }

      // TODO: what if accessors isn't an array?
      for(const nlohmann::json& accessor : gltf_json["accessors"]) {
        Accessor parsed_accessor{};

        if(accessor.contains("bufferView")) {
          parsed_accessor.buffer_view = accessor["bufferView"].get<size_t>();
        } else {
          ZEPHYR_PANIC("Unhandled accessor without buffer view");
        }

        if(accessor.contains("byteOffset")) {
          parsed_accessor.byte_offset = accessor["byteOffset"].get<size_t>();
        } else {
          parsed_accessor.byte_offset = 0u;
        }

        if(accessor.contains("componentType")) {
          parsed_accessor.component_type = accessor["componentType"].get<ComponentType>();
        } else {
          ZEPHYR_PANIC("Attribute 'componentType' missing from accessor");
        }

        if(accessor.contains("normalized")) {
          parsed_accessor.normalized = accessor["normalized"].get<bool>();
        } else {
          parsed_accessor.normalized = false;
        }

        if(accessor.contains("count")) {
          parsed_accessor.count = accessor["count"].get<size_t>();
        } else {
          ZEPHYR_PANIC("Attribute 'count' missing from accessor");
        }

        if(accessor.contains("type")) {
          const std::string type_string = accessor["type"].get<std::string>();

          if(type_string == "SCALAR") {
            parsed_accessor.type = AccessorType::Scalar;
          } else if(type_string == "VEC2") {
            parsed_accessor.type = AccessorType::Vec2;
          } else if(type_string == "VEC3") {
            parsed_accessor.type = AccessorType::Vec3;
          } else if(type_string == "VEC4") {
            parsed_accessor.type = AccessorType::Vec4;
          } else if(type_string == "MAT2") {
            parsed_accessor.type = AccessorType::Mat2;
          } else if(type_string == "MAT3") {
            parsed_accessor.type = AccessorType::Mat3;
          } else if(type_string == "MAT4") {
            parsed_accessor.type = AccessorType::Mat4;
          } else {
            ZEPHYR_PANIC("Bad accessor type: {}", type_string);
          }
        } else {
          ZEPHYR_PANIC("Attribute 'type' missing from accessor");
        }

        m_accessors.push_back(parsed_accessor);
      }
    }

    void LoadImages(const nlohmann::json& gltf_json) {
      if(!gltf_json.contains("images")) {
        return;
      }

      for(const nlohmann::json& image : gltf_json["images"]) {
        // TODO(fleroviux): assumes that URI is used and not an IRI or buffer view reference
        //const std::string uri = image["uri"].get<std::string>();
        const std::filesystem::path uri = m_base_path / image["uri"].get<std::string>();

        int width{};
        int height{};
        int channels_in_file{};
        u8* data = stbi_load(uri.string().c_str(), &width, &height, &channels_in_file, 3);
        if(channels_in_file != 3) {
          ZEPHYR_PANIC("failed to load image, expected three channels but got {}", channels_in_file);
        }
        std::shared_ptr<Texture2D> parsed_image = std::make_shared<Texture2D>(width, height);
        u8* src = data;
        u32* dst = parsed_image->Data<u32>();
        for(size_t i = 0; i < width * height; i++) {
          const u8 r = *src++;
          const u8 g = *src++;
          const u8 b = *src++;
          *dst++ = 0xFF000000u | r << 16 | g << 8 | b;
        }
        stbi_image_free(data);

        m_images.push_back(std::move(parsed_image));
      }
    }

    void LoadTextures(const nlohmann::json& gltf_json) {
      if(!gltf_json.contains("textures")) {
        return;
      }

      for(const nlohmann::json& texture : gltf_json["textures"]) {
        // TODO(fleroviux): source field isn't required!
        m_textures.push_back(m_images[texture["source"].get<size_t>()]);
      }
    }

    void LoadMaterials(const nlohmann::json& gltf_json) {
      if(!gltf_json.contains("materials")) {
        return;
      }

      // TODO: handle error when materials is not an array
      for(const nlohmann::json& material : gltf_json["materials"]) {
        std::shared_ptr<Material> parsed_material = std::make_shared<Material>();

        if(material.contains("pbrMetallicRoughness")) {
          const nlohmann::json& pbr_metallic_roughness = material["pbrMetallicRoughness"];
          if(pbr_metallic_roughness.contains("baseColorTexture")) {
            parsed_material->m_diffuse_map = m_textures[pbr_metallic_roughness["baseColorTexture"]["index"].get<size_t>()];
          }
        }

        m_materials.push_back(std::move(parsed_material));
      }
    }

    void LoadMeshes(const nlohmann::json& gltf_json) {
      if(!gltf_json.contains("meshes")) {
        return;
      }

      // TODO: handle error when meshes is not an array
      for(const nlohmann::json& mesh : gltf_json["meshes"]) {
        Mesh parsed_mesh{};

        if(mesh.contains("primitives")) {
          // TODO: handle error when primitives is not an array
          for(const nlohmann::json& primitive : mesh["primitives"]) {
            Mesh::Primitive parsed_primitive{};

            if(!primitive.contains("attributes")) {
              ZEPHYR_PANIC("Primitive is missing attributes map");
            }

            const nlohmann::json& attributes = primitive["attributes"];

            RenderGeometryLayout layout{};
            if(attributes.contains("POSITION"))   layout.AddAttribute(RenderGeometryAttribute::Position);
            if(attributes.contains("NORMAL"))     layout.AddAttribute(RenderGeometryAttribute::Normal);
//              if(attributes.contains("TEXCOORD_0")) layout.AddAttribute(RenderGeometryAttribute::UV);
//              if(attributes.contains("COLOR_0"))    layout.AddAttribute(RenderGeometryAttribute::Color);

            std::unique_ptr<Geometry> geometry = std::make_unique<Geometry>(layout, 0u, 0u);

            if(attributes.contains("POSITION")) {
              const size_t accessor_index = attributes["POSITION"];
              if(accessor_index >= m_accessors.size()) {
                ZEPHYR_PANIC("Out-of-bounds accessor index")
              }

              const Accessor& accessor = m_accessors[accessor_index];
              if(accessor.type != AccessorType::Vec3 || accessor.component_type != ComponentType::Float) {
                ZEPHYR_PANIC("POSITION attribute accessor has bad accessor type and component type combination ({}, {})", (int)accessor.type, (int)accessor.component_type);
              }

              // TODO(fleroviux): find a better way to solve this.
              geometry->SetNumberOfVertices(accessor.count);
              LoadVec3Accessor(accessor, geometry->GetPositions());
            }

            if(attributes.contains("NORMAL")) {
              const size_t accessor_index = attributes["NORMAL"];
              if(accessor_index >= m_accessors.size()) {
                ZEPHYR_PANIC("Out-of-bounds accessor index")
              }

              const Accessor& accessor = m_accessors[accessor_index];
              if(accessor.type != AccessorType::Vec3 || accessor.component_type != ComponentType::Float) {
                ZEPHYR_PANIC("NORMAL attribute accessor has bad accessor type and component type combination ({}, {})", (int)accessor.type, (int)accessor.component_type);
              }

              // TODO(fleroviux): find a better way to solve this.
              geometry->SetNumberOfVertices(accessor.count);
              LoadVec3Accessor(accessor, geometry->GetNormals());
            }

            if(primitive.contains("indices")) {
              const size_t accessor_index = primitive["indices"].get<size_t>();
              if(accessor_index >= m_accessors.size()) {
                ZEPHYR_PANIC("Out-of-bounds accessor index")
              }

              const Accessor& accessor = m_accessors[accessor_index];
              if(accessor.type != AccessorType::Scalar || (accessor.component_type != ComponentType::UnsignedShort && accessor.component_type != ComponentType::UnsignedInt)) {
                ZEPHYR_PANIC("indices accessor has bad accessor type and component type combination ({}, {})", (int)accessor.type, (int)accessor.component_type);
              }

              // TODO(fleroviux): find a better way to solve this.
              geometry->SetNumberOfIndices(accessor.count);
              LoadIndexBufferAccessor(accessor, geometry->GetIndices());
            }

            parsed_primitive.geometry = std::move(geometry);
            if(primitive.contains("material")) {
              parsed_primitive.material = m_materials[primitive["material"].get<size_t>()];
            } else {
              fmt::print("huh? primitive without material? how to deal with this?\n");
            }

            parsed_mesh.primitives.push_back(std::move(parsed_primitive));
          }
        } else {
          ZEPHYR_PANIC("Mesh is missing primitives array");
        }

        m_meshes.push_back(std::move(parsed_mesh));
      }
    }

    void LoadVec3Accessor(const Accessor& accessor, Geometry::Accessor<Vector3> geometry_accessor) {
      // TODO: support component types other than float
      // TODO: lots of validation... all of this is unsafe.

      const BufferView& buffer_view = m_buffer_views[accessor.buffer_view];
      const Buffer& buffer = m_buffers[buffer_view.buffer];
      const size_t stride = buffer_view.byte_stride == 0 ? sizeof(f32) * 3 : buffer_view.byte_stride; // Attribute is tightly packed if stride is zero

      uintptr_t data_address = (uintptr_t)buffer.data() + buffer_view.byte_offset + accessor.byte_offset;

      for(size_t i = 0; i < accessor.count; i++) {
        geometry_accessor[i] = *(const Vector3*)data_address;
        data_address += stride;
      }
    }

    void LoadIndexBufferAccessor(const Accessor& accessor, std::span<u32> geometry_index_buffer) {
      // TODO: lots of validation... all of this is unsafe.
      const BufferView& buffer_view = m_buffer_views[accessor.buffer_view];
      const Buffer& buffer = m_buffers[buffer_view.buffer];
      size_t stride = buffer_view.byte_stride;

      uintptr_t data_address = (uintptr_t)buffer.data() + buffer_view.byte_offset + accessor.byte_offset;

      switch(accessor.component_type) {
        case ComponentType::UnsignedShort: {
          if(stride == 0u) {
            stride = sizeof(u16);
          }
          for(size_t i = 0; i < accessor.count; i++) {
            geometry_index_buffer[i] = *(const u16*)data_address;
            data_address += stride;
          }
          break;
        }
        case ComponentType::UnsignedInt: {
          if(stride == 0u) {
            stride = sizeof(u32);
          }
          for(size_t i = 0; i < accessor.count; i++) {
            geometry_index_buffer[i] = *(const u32*)data_address;
            data_address += stride;
          }
          break;
        }
        default: ZEPHYR_PANIC("unsupported component type");
      }
    }

    std::shared_ptr<SceneNode> LoadNodeHierarchy(const nlohmann::json& gltf_json, size_t node_index) {
      // TODO(fleroviux): error handling and everything
      // TODO: handle graph cycles
      // TODO(fleroviux): load transform

      const nlohmann::json& node_json = gltf_json["nodes"][node_index];

      std::string name;

      if(node_json.contains("name")) {
        name = node_json["name"].get<std::string>();
      } else {
        name = fmt::format("GLTFNode{}", node_index);
      }

      std::shared_ptr<SceneNode> node = SceneNode::New(name);

      if(node_json.contains("mesh")) {
        Mesh& mesh = m_meshes[node_json["mesh"].get<size_t>()];
        if(mesh.primitives.size() == 1u) {
          node->CreateComponent<MeshComponent>(mesh.primitives[0].geometry, mesh.primitives[0].material);
        } else {
          for(size_t i = 0; i < mesh.primitives.size(); i++) {
            std::shared_ptr<SceneNode> primitive_node = node->CreateChild(fmt::format("{}#{}", name, i));
            primitive_node->CreateComponent<MeshComponent>(mesh.primitives[i].geometry, mesh.primitives[i].material);
          }
        }
      }

      if(node_json.contains("children")) {
        for(const nlohmann::json& child : node_json["children"]) {
          node->Add(LoadNodeHierarchy(gltf_json, child.get<size_t>()));
        }
      }

      return node;
    }

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


#pragma once

#include <zephyr/math/vector.hpp>
#include <zephyr/renderer/buffer/index_buffer.hpp>
#include <zephyr/renderer/buffer/vertex_buffer.hpp>
#include <zephyr/float.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <limits>

#include <zephyr/logger/logger.hpp>

namespace zephyr {

  /**
   * @todo:
   *   - support alpha-less color attribute
   *   - check if Mesh3D has attribute in GetAttribute and SetAttribute methods.
   *   - support for reading attributes
   *   - support for reading and writing indices
   *   - support for making VBO and IBO as dirty
   */

  class Mesh3D {
    public:
      enum class Attribute : u32 {
        Normal = 1,
        Color0 = 2,
        UV0 = 4
      };

      Mesh3D(size_t number_of_vertices, Attribute attributes)
          : m_number_of_vertices{number_of_vertices}
          , m_number_of_indices{0u}
          , m_attributes{attributes} {
        ComputeBufferLayout();
        CreateVBO();
        ComputeLayoutKey();
      }

      Mesh3D(size_t number_of_vertices, size_t number_of_indices, Attribute attributes)
          : m_number_of_vertices{number_of_vertices}
          , m_number_of_indices{number_of_indices}
          , m_attributes{attributes} {
        ComputeBufferLayout();
        CreateVBO();
        CreateIBO();
        ComputeLayoutKey();
      }

      [[nodiscard]] u32 GetLayoutKey() const {
        return m_layout_key;
      }

      [[nodiscard]] const VertexBuffer* GetVBO() const {
        return m_vbo.get();
      }

      [[nodiscard]] const IndexBuffer* GetIBO() const {
        return m_ibo.get();
      }

      void SetPosition(size_t id, Vector3 position) {
        m_vbo->Write<Vector3>(id, position, 0u);
      }

      void SetNormal(size_t id, Vector3 normal) {
        m_vbo->Write<Vector3>(id, normal, m_normal_offset);
      }

      void SetColor0(size_t id, Vector3 color) {
        m_vbo->Write<Vector4>(id, Vector4{color, 1.0f}, m_color0_offset);
      }

      void SetColor0(size_t id, Vector4 color) {
        m_vbo->Write<Vector4>(id, color, m_color0_offset);
      }

      void SetUV0(size_t id, Vector2 uv) {
        m_vbo->Write<Vector2>(id, uv, m_uv0_offset);
      }

      void SetIndex(size_t id, u32 index) {
        switch(m_ibo->GetDataType()) {
          case IndexDataType::UInt16: m_ibo->Data<u16>()[id] = index; break;
          case IndexDataType::UInt32: m_ibo->Data<u32>()[id] = index; break;
          default: ZEPHYR_PANIC("Unimplemented index data type");
        }
      }

    private:
      void ComputeBufferLayout() {
        // the position attribute always is implicitly present.
        size_t current_offset = sizeof(f32) * 3;

        if((u32)m_attributes & (u32)Attribute::Normal) {
          m_normal_offset = current_offset;
          current_offset += sizeof(f32) * 3;
        }

        if((u32)m_attributes & (u32)Attribute::Color0) {
          m_color0_offset = current_offset;
          current_offset += sizeof(f32) * 4;
        }

        if((u32)m_attributes & (u32)Attribute::UV0) {
          m_uv0_offset = current_offset;
          current_offset += sizeof(f32) * 2;
        }

        m_stride = current_offset;

        ZEPHYR_INFO("normal_offset={} color0_offset={} uv0_offset={} stride={}", m_normal_offset, m_color0_offset, m_uv0_offset, m_stride)
      }

      void CreateVBO() {
        m_vbo = std::make_unique<VertexBuffer>(m_stride, m_number_of_vertices);
      }

      void CreateIBO() {
        IndexDataType data_type;

        if(m_number_of_vertices <= std::numeric_limits<u16>::max() + 1u) {
          data_type = IndexDataType::UInt16;
        } else if(m_number_of_vertices <= std::numeric_limits<u32>::max() + 1ull) {
          data_type = IndexDataType::UInt32;
        } else {
          ZEPHYR_PANIC("Number of vertices ({}) fits into no known index data type", m_number_of_vertices);
        }

        m_ibo = std::make_unique<IndexBuffer>(data_type, m_number_of_indices);
      }

      void ComputeLayoutKey() {
        m_layout_key = (u32)m_attributes;

        if((bool)m_ibo) {
          m_layout_key |= 0x8000'0000u;

          if(m_ibo->GetDataType() == IndexDataType::UInt32) {
            m_layout_key |= 0x4000'0000u;
          }
        }
      }

      size_t m_number_of_vertices;
      size_t m_number_of_indices;
      Attribute m_attributes;

      size_t m_normal_offset{};
      size_t m_color0_offset{};
      size_t m_uv0_offset{};
      size_t m_stride{};

      std::unique_ptr<VertexBuffer> m_vbo;
      std::unique_ptr<IndexBuffer> m_ibo;
      u32 m_layout_key{};
  };

  constexpr auto operator|(Mesh3D::Attribute lhs, Mesh3D::Attribute rhs) {
    return (Mesh3D::Attribute)((u32)lhs | (u32)rhs);
  }

  constexpr auto operator&(Mesh3D::Attribute lhs, Mesh3D::Attribute rhs) {
    return (Mesh3D::Attribute)((u32)lhs & (u32)rhs);
  }

} // namespace zephyr


#pragma once

#include <zephyr/math/vector.hpp>
#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/resource/resource.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/float.hpp>
#include <array>
#include <cstdlib>
#include <span>

namespace zephyr {

  class Geometry final : public Resource {
    public:
      template<typename T>
      class Accessor {
        public:
          Accessor(void* data, size_t offset, size_t stride, size_t number_of_elements)
              : m_data{(u8*)data + offset}
              , m_stride{stride}
              , m_number_of_elements{number_of_elements} {
          }

          [[nodiscard]] bool IsValid() const {
            return m_data != nullptr;
          }

          [[nodiscard]] size_t GetNumberOfElements() const {
            return m_number_of_elements;
          }

          [[nodiscard]] T& operator[](size_t i) {
            return *(T*)(m_data + i * m_stride);
          }

        private:
          u8* m_data;
          size_t m_stride;
          size_t m_number_of_elements;
      };

      Geometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices = 0u)
          : m_layout{layout} {
        size_t next_attribute_offset = 0u;

        const auto RegisterAttribute = [&](RenderGeometryAttribute attribute, size_t number_of_components, size_t underlying_type_size) {
          if(layout.HasAttribute(attribute)) {
            m_attribute_offsets[(int)attribute] = next_attribute_offset;
            next_attribute_offset += number_of_components * underlying_type_size;
          }
        };

        RegisterAttribute(RenderGeometryAttribute::Position, 3, sizeof(f32));
        RegisterAttribute(RenderGeometryAttribute::Normal,   3, sizeof(f32));
        RegisterAttribute(RenderGeometryAttribute::UV,       2, sizeof(f32));
        RegisterAttribute(RenderGeometryAttribute::Color,    4, sizeof(f32));
        m_vertex_stride = next_attribute_offset;

        SetNumberOfVertices(number_of_vertices);
        SetNumberOfIndices(number_of_indices);
      }

     ~Geometry() override {
        if(m_index_data) {
          std::free(m_index_data);
        }

        if(m_vertex_data) {
          std::free(m_vertex_data);
        }
      }

      [[nodiscard]] bool HasAttribute(RenderGeometryAttribute attribute) const {
        return m_layout.HasAttribute(attribute);
      }

      [[nodiscard]] bool IsIndexed() const {
        return GetNumberOfIndices() > 0;
      }

      [[nodiscard]] size_t GetNumberOfIndices() const {
        return m_number_of_indices;
      }

      /// @note: this invalidates any previously created spans to the index data.
      void SetNumberOfIndices(size_t number_of_indices) {
        if(number_of_indices != m_number_of_indices) {
          m_index_data = (u32*)std::realloc(m_index_data, sizeof(u32) * number_of_indices);
          m_number_of_indices = number_of_indices;
        }
      }

      [[nodiscard]] size_t GetNumberOfVertices() const {
        return m_number_of_vertices;
      }

      /// @note: this invalidates any previously created accessors to the vertex data.
      void SetNumberOfVertices(size_t number_of_vertices) {
        if(number_of_vertices != m_number_of_vertices) {
          m_vertex_data = std::realloc(m_vertex_data, m_vertex_stride * number_of_vertices);
          m_number_of_vertices = number_of_vertices;
        }
      }

      [[nodiscard]] std::span<u32> GetIndices() {
        return {m_index_data, m_number_of_indices};
      }

      [[nodiscard]] Accessor<Vector3> GetPositions() {
        return GetAccessor<Vector3>(RenderGeometryAttribute::Position);
      }

      [[nodiscard]] Accessor<Vector3> GetNormals() {
        return GetAccessor<Vector3>(RenderGeometryAttribute::Normal);
      }

      [[nodiscard]] Accessor<Vector2> GetUVs() {
        return GetAccessor<Vector2>(RenderGeometryAttribute::UV);
      }

      [[nodiscard]] Accessor<Vector4> GetColors() {
        return GetAccessor<Vector4>(RenderGeometryAttribute::Color);
      }

      [[nodiscard]] RenderGeometryLayout GetLayout() const {
        return m_layout;
      }

      [[nodiscard]] std::span<const u8> GetRawIndexData() const {
        return {(const u8*)m_index_data, sizeof(u32) * m_number_of_indices};
      }

      [[nodiscard]] std::span<const u8> GetRawVertexData() const {
        return {(const u8*)m_vertex_data, m_vertex_stride * m_number_of_vertices};
      }

    private:
      template<typename T>
      [[nodiscard]] Accessor<T> GetAccessor(RenderGeometryAttribute attribute) {
        if(!HasAttribute(attribute)) {
          return {nullptr, 0u, 0u, 0u};
        }
        return {m_vertex_data, m_attribute_offsets[(int)attribute], m_vertex_stride, m_number_of_vertices};
      }

      u32* m_index_data{};
      size_t m_number_of_indices{};

      RenderGeometryLayout m_layout{};
      void* m_vertex_data{};
      size_t m_vertex_stride{};
      size_t m_number_of_vertices{};
      std::array<size_t, (int)RenderGeometryAttribute::Count> m_attribute_offsets{};
  };

} // namespace zephyr

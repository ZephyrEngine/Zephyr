
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <memory>
#include <span>
#include <unordered_map>

#include "dynamic_gpu_array.hpp"

namespace zephyr {

  struct OpenGLDrawElementsIndirectCommand {
    GLuint count;
    GLuint instance_count;
    GLuint first_index;
    GLint  base_vertex;
    GLuint base_instance;
  };

  struct OpenGLDrawArraysIndirectCommand {
    GLuint count;
    GLuint instance_count;
    GLuint first;
    GLuint base_instance;
  };

  class OpenGLRenderGeometry final : public RenderGeometry {
    public:
      OpenGLRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices, std::shared_ptr<OpenGLDynamicGPUArray> vbo, std::shared_ptr<OpenGLDynamicGPUArray> ibo)
          : m_layout{layout}
          , m_number_of_vertices{number_of_vertices}
          , m_number_of_indices{number_of_indices}
          , m_vbo{std::move(vbo)} {
        m_vbo_allocation = m_vbo->AllocateRange(number_of_vertices);

        if(number_of_indices > 0u) {
          m_ibo = std::move(ibo);
          m_ibo_allocation = m_ibo->AllocateRange(number_of_indices);
        }
      }

     ~OpenGLRenderGeometry() override {
        m_vbo->ReleaseRange(m_vbo_allocation);

        if(m_ibo) {
          m_ibo->ReleaseRange(m_ibo_allocation);
        }
      }

      [[nodiscard]] RenderGeometryLayout GetLayout() const {
        return m_layout;
      }

      [[nodiscard]] size_t GetNumberOfVertices() const override {
        return m_number_of_vertices;
      }

      [[nodiscard]] size_t GetNumberOfIndices() const override {
        return m_number_of_indices;
      }

      [[nodiscard]] OpenGLDrawElementsIndirectCommand GetDrawElementsIndirectCommand() const {
        return {
          .count = (GLuint)m_number_of_indices,
          .instance_count = 1u,
          .first_index = (GLuint)m_ibo_allocation.base_element,
          .base_vertex = (GLint)m_vbo_allocation.base_element,
          .base_instance = 0u,
        };
      }

      [[nodiscard]] OpenGLDrawArraysIndirectCommand GetDrawArraysIndirectCommand() const {
        return {
          .count = (GLuint)m_number_of_vertices,
          .instance_count = 1u,
          .first = (GLuint)m_vbo_allocation.base_element,
          .base_instance = 0u,
        };
      }

      void WriteVBO(std::span<const u8> data) {
        // TODO(fleroviux): this does not protect against writing into neighbouring allocations.
        m_vbo->Write(data, m_vbo_allocation.base_element);
      }

      void WriteIBO(std::span<const u8> data) {
        // TODO(fleroviux): this does not protect against writing into neighbouring allocations.
        if(!m_ibo) {
          ZEPHYR_PANIC("Attempted to write IBO of non-indexed render geometry");
        }
        m_ibo->Write(data, m_ibo_allocation.base_element);
      }

    private:
      RenderGeometryLayout m_layout;
      size_t m_number_of_vertices;
      size_t m_number_of_indices;
      std::shared_ptr<OpenGLDynamicGPUArray> m_vbo;
      std::shared_ptr<OpenGLDynamicGPUArray> m_ibo{};
      OpenGLDynamicGPUArray::BufferRange m_vbo_allocation{};
      OpenGLDynamicGPUArray::BufferRange m_ibo_allocation{};
  };

  class OpenGLRenderGeometryManager {
    public:
      OpenGLRenderGeometryManager() {
        m_ibo = std::make_shared<OpenGLDynamicGPUArray>(sizeof(u32));
      }

      GLuint GetVAOFromLayout(RenderGeometryLayout layout) {
        // TODO(fleroviux): do not create the bucket if it does not exist.
        // TODO(fleroviux): avoid unnecessary rebinding of the vertex and index buffers.
        Bucket& bucket = GetBucketFromLayout(layout);
        glVertexArrayVertexBuffer(bucket.vao, 0u, bucket.vbo->GetBufferHandle(), 0u, (GLsizei)bucket.vbo->GetByteStride());
        glVertexArrayElementBuffer(bucket.vao, m_ibo->GetBufferHandle());
        return bucket.vao;
      }

      RenderGeometry* CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) {
        const Bucket& bucket = GetBucketFromLayout(layout);
        return new OpenGLRenderGeometry{layout, number_of_vertices, number_of_indices, bucket.vbo, m_ibo};
      }

      void UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) {
        dynamic_cast<OpenGLRenderGeometry*>(render_geometry)->WriteIBO(data);
      }

      void UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) {
        dynamic_cast<OpenGLRenderGeometry*>(render_geometry)->WriteVBO(data);
      }

      void DestroyRenderGeometry(RenderGeometry* render_geometry) {
        delete render_geometry;
      }

    private:
      struct Bucket {
       ~Bucket() { glDeleteVertexArrays(1u, &vao); }
        GLuint vao{};
        std::shared_ptr<OpenGLDynamicGPUArray> vbo{};
      };

      Bucket& GetBucketFromLayout(RenderGeometryLayout layout) {
        Bucket& bucket = m_layout_to_bucket_table[layout.key];

        if(!bucket.vbo) {
          size_t next_attribute_offset = 0u;

          const auto RegisterAttribute = [&](RenderGeometryAttribute attribute, int number_of_components) {
            if(!layout.HasAttribute(attribute)) {
              return;
            }
            glEnableVertexArrayAttrib(bucket.vao, (int)attribute);
            glVertexArrayAttribFormat(bucket.vao, (int)attribute, number_of_components, GL_FLOAT, GL_FALSE, next_attribute_offset);
            glVertexArrayAttribBinding(bucket.vao, (int)attribute, 0u);
            next_attribute_offset += sizeof(f32) * number_of_components;
          };

          glCreateVertexArrays(1u, &bucket.vao);
          RegisterAttribute(RenderGeometryAttribute::Position, 3);
          RegisterAttribute(RenderGeometryAttribute::Normal, 3);
          RegisterAttribute(RenderGeometryAttribute::UV, 2);
          RegisterAttribute(RenderGeometryAttribute::Color, 3);

          const size_t byte_stride = next_attribute_offset;
          bucket.vbo = GetVBOFromByteStride(byte_stride);
        }

        return bucket;
      }

      std::shared_ptr<OpenGLDynamicGPUArray> GetVBOFromByteStride(size_t byte_stride) {
        if(!m_layout_to_bucket_table.contains(byte_stride)) {
          m_byte_stride_to_vbo_table[byte_stride] = std::make_shared<OpenGLDynamicGPUArray>(byte_stride);
        }
        return m_byte_stride_to_vbo_table[byte_stride];
      }

      std::shared_ptr<OpenGLDynamicGPUArray> m_ibo{};
      std::unordered_map<size_t, std::shared_ptr<OpenGLDynamicGPUArray>> m_byte_stride_to_vbo_table{};
      std::unordered_map<decltype(RenderGeometryLayout::key), Bucket> m_layout_to_bucket_table{};
  };

} // namespace zephyr
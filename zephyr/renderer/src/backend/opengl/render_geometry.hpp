
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/integer.hpp>
#include <optional>
#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL_opengl.h>

namespace zephyr {

  class OpenGLRenderGeometry final : public RenderGeometry {
    public:
      static OpenGLRenderGeometry* Build(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) {
        GLuint gl_vao;
        glCreateVertexArrays(1u, &gl_vao);

        std::optional<GLuint> maybe_gl_ibo{};

        if(number_of_indices > 0) {
          // @todo: use GL_STATIC_DRAW when possible.
          GLuint gl_ibo;
          glCreateBuffers(1u, &gl_ibo);
          glNamedBufferData(gl_ibo, (GLsizeiptr)(sizeof(u32) * number_of_indices), nullptr, GL_DYNAMIC_DRAW);
          glVertexArrayElementBuffer(gl_vao, gl_ibo);
          maybe_gl_ibo = gl_ibo;
        }

        size_t vbo_stride = 0u;
        std::array<size_t, 32> vbo_attribute_offsets{};

        const auto PackAttribute = [&](RenderGeometryAttribute attribute, int number_of_components) {
          if(layout.HasAttribute(attribute)) {
            glEnableVertexArrayAttrib(gl_vao, (int)attribute);
            glVertexArrayAttribFormat(gl_vao, (int)attribute, number_of_components, GL_FLOAT, GL_FALSE, vbo_stride);
            glVertexArrayAttribBinding(gl_vao, (int)attribute, 0u);

            vbo_attribute_offsets[(int)attribute] = vbo_stride;
            vbo_stride += sizeof(f32) * number_of_components;
          }
        };

        PackAttribute(RenderGeometryAttribute::Position, 3);
        PackAttribute(RenderGeometryAttribute::Normal, 3);
        PackAttribute(RenderGeometryAttribute::UV, 2);
        PackAttribute(RenderGeometryAttribute::Color, 4);

        // @todo: use GL_STATIC_DRAW when possible.
        GLuint gl_vbo;
        glCreateBuffers(1u, &gl_vbo);
        glNamedBufferData(gl_vbo, (GLsizeiptr)(vbo_stride * number_of_vertices), nullptr, GL_DYNAMIC_DRAW);
        glVertexArrayVertexBuffer(gl_vao, 0u, gl_vbo, 0u, (GLsizei)vbo_stride);

        OpenGLRenderGeometry* render_geometry = new OpenGLRenderGeometry{};
        render_geometry->m_gl_vao = gl_vao;
        render_geometry->m_gl_ibo = maybe_gl_ibo;
        render_geometry->m_gl_vbo = gl_vbo;
        render_geometry->m_vbo_stride = vbo_stride;
        render_geometry->m_vbo_attribute_offsets = vbo_attribute_offsets;
        render_geometry->m_number_of_indices = number_of_indices;
        render_geometry->m_number_of_vertices = number_of_vertices;
        return render_geometry;
      }

      [[nodiscard]] size_t GetNumberOfVertices() const override {
        return m_number_of_vertices;
      }

      [[nodiscard]] size_t GetNumberOfIndices() const override {
        return m_number_of_indices;
      }

      void UpdateIndices(std::span<const u8> data) {
        // @todo: validation
        glNamedBufferSubData(m_gl_ibo.value(), 0u, (GLsizeiptr)data.size_bytes(), data.data());
      }

      void UpdateVertices(std::span<const u8> data) {
        //@todo: validation
        glNamedBufferSubData(m_gl_vbo, 0u, (GLsizeiptr)data.size_bytes(), data.data());
      }

      void Draw() {
        glBindVertexArray(m_gl_vao);

        if(m_gl_ibo.has_value()) {
          glDrawElements(GL_TRIANGLES, (GLsizei)m_number_of_indices, GL_UNSIGNED_INT, nullptr);
        } else {
          glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_number_of_vertices);
        }
      }

    private:
      static_assert((int)RenderGeometryAttribute::Count <= 32);

      OpenGLRenderGeometry() = default;

      GLuint m_gl_vao{};
      std::optional<GLuint> m_gl_ibo{};
      GLuint m_gl_vbo{};
      size_t m_vbo_stride{};
      std::array<size_t, 32> m_vbo_attribute_offsets{};
      size_t m_number_of_indices{};
      size_t m_number_of_vertices{};
  };

} // namespace zephyr

#pragma once

#include <zephyr/math/box3.hpp>
#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <limits>
#include <memory>
#include <span>

#include "backend/opengl/dynamic_gpu_array.hpp"

namespace zephyr {

class OpenGLRenderGeometry final : public RenderGeometry {
  public:
    struct RenderData {
      Vector4 aabb_min;
      Vector4 aabb_max;
      u32 mdi_command[5]; // Stores either a DrawElementsIndirectCommand or DrawArraysIndirectCommand
      u32 padding[3]; // Padding for std430 layout
    };

    OpenGLRenderGeometry(
      RenderGeometryLayout layout,
      size_t number_of_vertices,
      size_t number_of_indices,
      std::shared_ptr<OpenGLDynamicGPUArray> vbo,
      std::shared_ptr<OpenGLDynamicGPUArray> ibo,
      std::shared_ptr<OpenGLDynamicGPUArray> draw_command_buffer
    );

   ~OpenGLRenderGeometry() override;

    [[nodiscard]] RenderGeometryLayout GetLayout() const override {
      return m_layout;
    }

    [[nodiscard]] size_t GetGeometryID() const override {
      return m_geometry_render_data_allocation.base_element;
    }

    [[nodiscard]] size_t GetNumberOfVertices() const override {
      return m_vbo_allocation.number_of_elements;
    }

    [[nodiscard]] size_t GetNumberOfIndices() const override {
      return m_ibo_allocation.number_of_elements;
    }

    void WriteVBO(std::span<const u8> data);
    void WriteIBO(std::span<const u8> data);
    void SetAABB(const Box3& aabb);

  private:
    void WriteGeometryRenderDataToBuffer();

    RenderGeometryLayout m_layout;
    std::shared_ptr<OpenGLDynamicGPUArray> m_vbo;
    std::shared_ptr<OpenGLDynamicGPUArray> m_ibo{};
    std::shared_ptr<OpenGLDynamicGPUArray> m_geometry_render_data_buffer{};
    OpenGLDynamicGPUArray::BufferRange m_vbo_allocation{};
    OpenGLDynamicGPUArray::BufferRange m_ibo_allocation{};
    OpenGLDynamicGPUArray::BufferRange m_geometry_render_data_allocation{};
    RenderData m_geometry_render_data{};
};

} // namespace zephyr
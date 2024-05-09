
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <span>
#include <vector>

namespace zephyr {

  /**
   * TODO:
   * - improve grow strategy (i.e. configurable grow factor)
   * - implement shrinking
   */

  class OpenGLDynamicGPUArray {
    public:
      struct BufferRange {
        BufferRange() = default;
        BufferRange(size_t base_element, size_t number_of_elements) : base_element{base_element}, number_of_elements{number_of_elements} {}
        size_t base_element;
        size_t number_of_elements;
      };

      explicit OpenGLDynamicGPUArray(size_t byte_stride);
     ~OpenGLDynamicGPUArray();

      [[nodiscard]] GLuint GetBufferHandle() {
        return m_gpu_buffer;
      }

      [[nodiscard]] size_t GetByteStride() const {
        return m_byte_stride;
      }

      BufferRange AllocateRange(size_t number_of_elements);
      void ReleaseRange(BufferRange buffer_range);
      void Write(std::span<const u8> data, size_t base_element, size_t byte_offset = 0u);

    private:
      static constexpr size_t k_capacity_increment = 16384u;

      void ResizeBuffer(size_t new_capacity);
      void PrintFreeRanges();

      size_t m_byte_stride{};
      GLuint m_gpu_buffer{};
      size_t m_current_capacity{0u};
      std::vector<BufferRange> m_free_buffer_ranges{};
  };

} // namespace zephyr

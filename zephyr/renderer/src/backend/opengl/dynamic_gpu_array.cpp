
#include <zephyr/float.hpp>
#include <zephyr/panic.hpp>
#include <algorithm>

#include <fmt/format.h>

#include "dynamic_gpu_array.hpp"

namespace zephyr {

OpenGLDynamicGPUArray::OpenGLDynamicGPUArray(size_t byte_stride) : m_byte_stride{byte_stride} {
  ResizeBuffer(k_capacity_increment);

//    const auto PrintAlloc = [](const BufferRange& range) {
//      fmt::print("allocated: base_element={} \tnumber_of_elements={}\n", range.base_element, range.number_of_elements);
//    };
//
//    const auto range_a = AllocateRange(512);
//    PrintAlloc(range_a);
//    PrintFreeRanges();
//
//    const auto range_b = AllocateRange(16385);
//    PrintAlloc(range_b);
//    PrintFreeRanges();
//
//    const auto range_c = AllocateRange(100000);
//    PrintAlloc(range_c);
//    PrintFreeRanges();
//
//    ReleaseRange(range_b);
//    PrintFreeRanges();
//
//    ReleaseRange(range_c);
//    PrintFreeRanges();
//
//    ReleaseRange(range_a);
//    PrintFreeRanges();
}

OpenGLDynamicGPUArray::~OpenGLDynamicGPUArray() {
  glDeleteBuffers(1u, &m_gpu_buffer);
}

OpenGLDynamicGPUArray::BufferRange OpenGLDynamicGPUArray::AllocateRange(size_t number_of_elements) {
  const auto end = m_free_buffer_ranges.end();

  for(auto it = m_free_buffer_ranges.begin(); it != end; ++it) {
    BufferRange& free_buffer_range = *it;

    if(free_buffer_range.number_of_elements >= number_of_elements) {
      const BufferRange allocated_range{free_buffer_range.base_element, number_of_elements};

      if(free_buffer_range.number_of_elements == number_of_elements) {
        m_free_buffer_ranges.erase(it);
      } else {
        free_buffer_range.base_element += number_of_elements;
        free_buffer_range.number_of_elements -= number_of_elements;
      }

      return allocated_range;
    }
  }

  const size_t required_capacity = m_current_capacity + number_of_elements - m_free_buffer_ranges.back().number_of_elements;
  const size_t  rounded_capacity = (required_capacity + k_capacity_increment - 1u) / k_capacity_increment * k_capacity_increment;
  ResizeBuffer(rounded_capacity);

  BufferRange& free_buffer_range = m_free_buffer_ranges.back();
  const BufferRange allocated_range{free_buffer_range.base_element, number_of_elements};

  if(free_buffer_range.number_of_elements == number_of_elements) {
    m_free_buffer_ranges.pop_back();
  } else {
    free_buffer_range.base_element += number_of_elements;
    free_buffer_range.number_of_elements -= number_of_elements;
  }

  return allocated_range;
}

void OpenGLDynamicGPUArray::ReleaseRange(BufferRange free_buffer_range) {
  fmt::print("release: base={} \tnumber_of_elements={}\n", free_buffer_range.base_element, free_buffer_range.number_of_elements);

  auto neighbour_r_it = std::ranges::find_if(m_free_buffer_ranges, [&](const BufferRange& buffer_range) {
    return buffer_range.base_element > free_buffer_range.base_element; });

  std::vector<BufferRange>::iterator free_range_it;

  if(neighbour_r_it != m_free_buffer_ranges.end()) {
    if(neighbour_r_it->base_element == free_buffer_range.base_element + free_buffer_range.number_of_elements) {
      neighbour_r_it->base_element = free_buffer_range.base_element;
      neighbour_r_it->number_of_elements += free_buffer_range.number_of_elements;
      free_range_it = neighbour_r_it;
    } else {
      free_range_it = m_free_buffer_ranges.insert(neighbour_r_it, free_buffer_range);
    }
  } else {
    m_free_buffer_ranges.push_back(free_buffer_range);
    free_range_it = m_free_buffer_ranges.end();
  }

  if(free_range_it != m_free_buffer_ranges.begin()) {
    const auto neighbour_l_it = std::prev(free_range_it);

    if(free_range_it->base_element == neighbour_l_it->base_element + neighbour_l_it->number_of_elements) {
      neighbour_l_it->number_of_elements += free_range_it->number_of_elements;
      m_free_buffer_ranges.erase(free_range_it);
    }
  }

  // TODO(fleroviux): shrink array if possible
}

void OpenGLDynamicGPUArray::Write(std::span<const u8> data, size_t base_element, size_t byte_offset) {
  const size_t buffer_write_start = base_element * m_byte_stride + byte_offset;
  const size_t buffer_write_end = buffer_write_start + data.size();
  const size_t buffer_byte_size = m_current_capacity * m_byte_stride;

  if(buffer_write_end < buffer_write_start || buffer_write_end > buffer_byte_size) {
    ZEPHYR_PANIC("Out-of-range dynamic GPU array write");
  }

  glNamedBufferSubData(m_gpu_buffer, (GLintptr)buffer_write_start, (GLsizeiptr)data.size(), data.data());
}

void OpenGLDynamicGPUArray::PrintFreeRanges() {
  fmt::print("----------------------------------------\n");

  for(const BufferRange& free_buffer_range : m_free_buffer_ranges) {
    fmt::print("free: \tbase={} \tnumber_of_elements={}\n", free_buffer_range.base_element, free_buffer_range.number_of_elements);
  }

  fmt::print("----------------------------------------\n");
}

void OpenGLDynamicGPUArray::ResizeBuffer(size_t new_capacity) {
  if(new_capacity == m_current_capacity) {
    return;
  }

  // Create a new VBO that fits the new capacity
  GLuint new_gpu_buffer;
  glCreateBuffers(1u, &new_gpu_buffer);
  glNamedBufferData(new_gpu_buffer, (GLsizeiptr)(new_capacity * m_byte_stride), nullptr, GL_DYNAMIC_DRAW);

  if(m_current_capacity != 0u) {
    // Copy the contents of the old buffer into the new buffer and delete the old buffer.
    const size_t copy_size = std::min(new_capacity, m_current_capacity) * m_byte_stride;
    glCopyNamedBufferSubData(m_gpu_buffer, new_gpu_buffer, 0u, 0u, (GLsizeiptr)copy_size);
    glDeleteBuffers(1u, &m_gpu_buffer);
  }

  if(new_capacity > m_current_capacity) {
    const size_t capacity_increment = new_capacity - m_current_capacity;

    if(m_free_buffer_ranges.empty()) {
      m_free_buffer_ranges.emplace_back(m_current_capacity, capacity_increment);
    } else {
      m_free_buffer_ranges.back().number_of_elements += capacity_increment;
    }
  }

  m_gpu_buffer = new_gpu_buffer;
  m_current_capacity = new_capacity;

  // TODO(fleroviux): emit event for handling rebinding of the new buffer to i.e. a VAO?
}

} // namespace zephyr

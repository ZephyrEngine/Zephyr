
#pragma once

#include <zephyr/gpu/enums.hpp>
#include <zephyr/renderer/resource/index_buffer.hpp>
#include <zephyr/renderer/resource/vertex_buffer.hpp>
#include <zephyr/renderer/resource/resource.hpp>
#include <zephyr/panic.hpp>
#include <zephyr/vector_n.hpp>
#include <span>

namespace zephyr {

  class Geometry final : public RendererResource {
    public:
      enum class Topology {
        Triangles
      };

      struct Attribute {
        size_t location;
        VertexDataType data_type;
        size_t components;
        bool normalized;
        size_t offset;
      };

      struct VertexBufferBinding {
        std::shared_ptr<VertexBuffer> buffer;
        Vector_N<Attribute, 32> attributes;

        [[nodiscard]] bool operator==(const std::shared_ptr<VertexBuffer>& other_buffer) const {
          return buffer == other_buffer;
        }
      };

      [[nodiscard]] Topology GetTopology() const {
        return m_topology;
      }

      void SetTopology(Topology topology) {
        m_topology = topology;
      }

      [[nodiscard]] const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const {
        return m_index_buffer;
      }

      void SetIndexBuffer(std::shared_ptr<IndexBuffer> buffer) {
        m_index_buffer = std::move(buffer);
      }

      [[nodiscard]] std::span<const VertexBufferBinding> GetVertexBufferBindings() const {
        return {m_vertex_buffer_bindings.Data(), m_vertex_buffer_bindings.Size()};
      }

      void UnbindVertexBuffer(const std::shared_ptr<VertexBuffer>& buffer) {
        const auto match = std::find(m_vertex_buffer_bindings.begin(), m_vertex_buffer_bindings.end(), buffer);

        if(match == m_vertex_buffer_bindings.end()) {
          ZEPHYR_PANIC("Geometry: attempted to unbind a vertex buffer that is not bound.");
        }

        m_vertex_buffer_bindings.Erase(match);
      }

      void BindVertexBuffer(std::shared_ptr<VertexBuffer> buffer, std::span<Attribute> attributes) {
        const auto match = std::find(m_vertex_buffer_bindings.begin(), m_vertex_buffer_bindings.end(), buffer);

        if(match != m_vertex_buffer_bindings.end()) {
          ZEPHYR_PANIC("Geometry: attempted to bind a vertex buffer that already is bound.")
        }

        m_vertex_buffer_bindings.PushBack({std::move(buffer), {attributes.begin(), attributes.end()}});
      }

    private:
      Topology m_topology{Topology::Triangles};
      std::shared_ptr<IndexBuffer> m_index_buffer;
      Vector_N<VertexBufferBinding, 32> m_vertex_buffer_bindings;
  };

} // namespace zephyr

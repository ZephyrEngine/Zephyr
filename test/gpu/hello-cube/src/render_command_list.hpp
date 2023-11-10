
#include <zephyr/gpu/render_device.hpp>
#include <zephyr/integer.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace zephyr::gpu_next {

  enum class RenderCommandID : u16 {
    BeginRenderPass,
    EndRenderPass,
    BindPipeline,
    BindVertexBuffer,
    BindIndexBuffer,
    PushConstants,
    DrawIndexed
  };

  class RenderCommandBase {
    public:
      explicit RenderCommandBase(RenderCommandID command_id) : m_command_id{command_id} {}

      [[nodiscard]] RenderCommandID CommandID() const {
        return m_command_id;
      }

      [[nodiscard]] RenderCommandBase* Next() const {
        return m_next;
      }

      void SetNext(RenderCommandBase* next) {
        m_next = next;
      }

    private:
      RenderCommandID m_command_id;
      RenderCommandBase* m_next{};
  };

  struct BeginRenderPassCmd : RenderCommandBase {
    BeginRenderPassCmd() : RenderCommandBase{RenderCommandID::BeginRenderPass} {}

    RenderTarget* render_target;
    RenderPass* render_pass;
  };

  // @todo: this is kind of superfluous - make this redundant!
  struct EndRenderPassCmd : RenderCommandBase {
    EndRenderPassCmd() : RenderCommandBase{RenderCommandID::EndRenderPass} {}
  };

  struct BindPipelineCmd : RenderCommandBase {
    BindPipelineCmd() : RenderCommandBase{RenderCommandID::BindPipeline} {}

    GraphicsPipeline* pipeline;
  };

  struct BindVertexBufferCmd : RenderCommandBase {
    BindVertexBufferCmd() : RenderCommandBase{RenderCommandID::BindVertexBuffer} {}

    u32 index;
    Buffer* buffer;
  };

  struct BindIndexBufferCmd : RenderCommandBase {
    BindIndexBufferCmd() : RenderCommandBase{RenderCommandID::BindIndexBuffer} {}

    IndexDataType data_type;
    Buffer* buffer;
  };

  struct PushConstantsCmd : RenderCommandBase {
    PushConstantsCmd() : RenderCommandBase{RenderCommandID::PushConstants} {}

    PipelineLayout* layout;
    u32 offset;
    u32 size;
    u8 data[128];
  };

  struct DrawIndexedCmd : RenderCommandBase {
    DrawIndexedCmd() : RenderCommandBase{RenderCommandID::DrawIndexed} {}

    u32 index_count;
  };

  class RenderCommandMemoryBlock {
    public:
      RenderCommandMemoryBlock() {
        // @todo: try to allocate close by
        //m_memory = new u8[k_bytes_per_block];
        m_memory = (u8*)VirtualAlloc(nullptr, k_bytes_per_block, MEM_COMMIT, PAGE_READWRITE);
        Reset();
      }

     ~RenderCommandMemoryBlock() {
        //delete[] m_memory;
        VirtualFree(m_memory, k_bytes_per_block, MEM_RELEASE);
      }

      void Reset() {
        m_current = m_memory;
      }

      u8* Allocate(size_t size) {
        u8* const base_address = m_current;
        u8* const stop_address = base_address + size;

        if(stop_address > m_memory + k_bytes_per_block) {
          return nullptr;
        }

        m_current = stop_address;
        return base_address;
      }

    private:
      static constexpr size_t k_bytes_per_block = 16384;

      u8* m_memory{};
      u8* m_current{};
  };

  class RenderCommandPool {
    public:
      RenderCommandPool() {
        m_memory_blocks.reserve(256);
        m_memory_blocks.emplace_back();
        m_current_block_index = 0u;
      }

      void Reset() {
        m_current_block_index = 0u;
        m_memory_blocks[0].Reset();
        // @todo: shrink our list of memory blocks at some point.
      }

      template<class T>
      T* Allocate() {
        u8* memory = m_memory_blocks[m_current_block_index].Allocate(sizeof(T));

        if(memory == nullptr) {
          m_current_block_index++;

          if(m_current_block_index >= m_memory_blocks.size()) {
            m_memory_blocks.emplace_back();
          } else {
            m_memory_blocks[m_current_block_index].Reset();
          }

          memory = m_memory_blocks[m_current_block_index].Allocate(sizeof(T));
        }

        return new(memory) T;
      }

    private:
      size_t m_current_block_index{};
      std::vector<RenderCommandMemoryBlock> m_memory_blocks{};
  };

  class RenderCommandList {
    public:
      void Reset() {
        m_command_pool.Reset();
        m_head = nullptr;
        m_tail = nullptr;
      }

      RenderCommandBase* GetHead() {
        return m_head;
      }

      void BeginRenderPass(RenderTarget* render_target, RenderPass* render_pass) {
        BeginRenderPassCmd* command = m_command_pool.Allocate<BeginRenderPassCmd>();
        command->render_target = render_target;
        command->render_pass = render_pass;
        Push(command);
      }

      void EndRenderPass() {
        Push(m_command_pool.Allocate<EndRenderPassCmd>());
      }

      void BindPipeline(GraphicsPipeline* pipeline) {
        BindPipelineCmd* command = m_command_pool.Allocate<BindPipelineCmd>();
        command->pipeline = pipeline;
        Push(command);
      }

      void BindVertexBuffer(u32 index, Buffer* buffer) {
        BindVertexBufferCmd* command = m_command_pool.Allocate<BindVertexBufferCmd>();
        command->index = index;
        command->buffer = buffer;
        Push(command);
      }

      void BindIndexBuffer(IndexDataType data_type, Buffer* buffer) {
        BindIndexBufferCmd* command = m_command_pool.Allocate<BindIndexBufferCmd>();
        command->data_type = data_type;
        command->buffer = buffer;
        Push(command);
      }

      void PushConstants(PipelineLayout* layout, u32 offset, u32 size, const void* data) {
        PushConstantsCmd* command = m_command_pool.Allocate<PushConstantsCmd>();
        command->layout = layout;
        command->offset = offset;
        command->size = size;
        std::memcpy(command->data, data, size); // insecure!
        Push(command);
      }

      void DrawIndexed(u32 index_count) {
        DrawIndexedCmd* command = m_command_pool.Allocate<DrawIndexedCmd>();
        command->index_count = index_count;
        Push(command);
      }

    private:
      void Push(RenderCommandBase* command) {
        if(m_head == nullptr) {
          m_head = command;
          m_tail = command;
        } else {
          m_tail->SetNext(command);
          m_tail = command;
        }
      }

      RenderCommandPool m_command_pool{};
      RenderCommandBase* m_head{};
      RenderCommandBase* m_tail{};
  };

} // namespace zephyr::gpu_next

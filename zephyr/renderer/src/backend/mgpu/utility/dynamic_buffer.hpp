
#pragma once

#include "mgpu/mgpu.h"
#include "zephyr/integer.hpp"
#include "zephyr/result.hpp"
#include <memory>

namespace zephyr {

// TODO: class naming is confusing with MGPU object names
class MGPUDynamicBuffer {
  public:
    static Result<MGPUResult, std::unique_ptr<MGPUDynamicBuffer>> Create(
      MGPUDevice mgpu_device,
      u64 byte_size,
      MGPUBufferUsage usage,
      MGPUBufferFlags flags
    );

   ~MGPUDynamicBuffer();

    [[nodiscard]] MGPUBuffer CurrentHandle() { return m_mgpu_buffer; }
    [[nodiscard]] u64 CurrentSize() const { return m_byte_size; }

    MGPUResult Resize(u64 new_byte_size);

  private:
    MGPUDynamicBuffer(MGPUDevice mgpu_device, MGPUBuffer mgpu_buffer, u64 byte_size, MGPUBufferUsage usage, MGPUBufferFlags flags);

    MGPUDevice m_mgpu_device;
    MGPUBuffer m_mgpu_buffer;
    u64 m_byte_size;
    MGPUBufferUsage m_usage;
    MGPUBufferFlags m_flags;
};

} // namespace zephyr

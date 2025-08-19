
#include "dynamic_buffer.hpp"

namespace zephyr {

MGPUDynamicBuffer::MGPUDynamicBuffer(
  MGPUDevice mgpu_device,
  MGPUBuffer mgpu_buffer,
  u64 byte_size,
  MGPUBufferUsage usage,
  MGPUBufferFlags flags
)   : m_mgpu_device{mgpu_device}
    , m_mgpu_buffer{mgpu_buffer}
    , m_byte_size{byte_size}
    , m_usage{usage}
    , m_flags{flags} {
}

MGPUDynamicBuffer::~MGPUDynamicBuffer() {
  mgpuBufferDestroy(m_mgpu_buffer);
}

Result<MGPUResult, std::unique_ptr<MGPUDynamicBuffer>> MGPUDynamicBuffer::Create(
  MGPUDevice mgpu_device,
  u64 byte_size,
  MGPUBufferUsage usage,
  MGPUBufferFlags flags
) {
  const MGPUBufferCreateInfo create_info{
    .size = byte_size,
    .usage = usage,
    .flags = flags
  };

  MGPUBuffer mgpu_buffer{};
  MGPUResult mgpu_result = mgpuDeviceCreateBuffer(mgpu_device, &create_info, &mgpu_buffer);
  if(mgpu_result != MGPU_SUCCESS) [[unlikely]] {
    return mgpu_result;
  }

  const auto dynamic_buffer = new (std::nothrow) MGPUDynamicBuffer{mgpu_device, mgpu_buffer, byte_size, usage, flags};
  if(dynamic_buffer == nullptr) [[unlikely]] {
    return MGPU_OUT_OF_MEMORY;
  }
  return std::unique_ptr<MGPUDynamicBuffer>{dynamic_buffer};
}

MGPUResult MGPUDynamicBuffer::Resize(u64 new_byte_size) {
  if(new_byte_size == m_byte_size) {
    return MGPU_SUCCESS;
  }

  const MGPUBufferCreateInfo create_info{
    .size = new_byte_size,
    .usage = m_usage,
    .flags = m_flags
  };

  MGPUBuffer mgpu_buffer{};
  MGPUResult mgpu_result = mgpuDeviceCreateBuffer(m_mgpu_device, &create_info, &mgpu_buffer);
  if(mgpu_result != MGPU_SUCCESS) [[unlikely]] {
    return mgpu_result;
  }

  mgpuBufferDestroy(m_mgpu_buffer);
  m_mgpu_buffer = mgpu_buffer;
  m_byte_size = new_byte_size;
  return MGPU_SUCCESS;
}

} // namespace zephyr

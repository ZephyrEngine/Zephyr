
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>

#include "render_backend.hpp"

namespace zephyr {

MGPURenderBackend::MGPURenderBackend(MGPUInstance mgpu_instance, MGPUSurface mgpu_surface)
    : m_mgpu_instance{mgpu_instance}
    , m_mgpu_surface{mgpu_surface} {
}

void MGPURenderBackend::InitializeContext() {
  MGPU_CHECK(mgpuInstanceSelectPhysicalDevice(m_mgpu_instance, MGPU_POWER_PREFERENCE_HIGH_PERFORMANCE, &m_mgpu_physical_device));
  if(m_mgpu_physical_device == nullptr) {
    ZEPHYR_PANIC("failed to find a suitable GPU");
  }

  MGPU_CHECK(mgpuPhysicalDeviceCreateDevice(m_mgpu_physical_device, &m_mgpu_device));

  m_mgpu_queue = mgpuDeviceGetQueue(m_mgpu_device, MGPU_QUEUE_TYPE_GRAPHICS_COMPUTE);

  MGPU_CHECK(mgpuDeviceCreateCommandList(m_mgpu_device, &m_mgpu_command_list));

  m_mgpu_test_dynamic_buffer = MGPUDynamicBuffer::Create(m_mgpu_device, 1024u, MGPU_BUFFER_USAGE_COPY_DST, 0).Unwrap();
  m_mgpu_test_dynamic_buffer->Resize(2048u);

  CreateSwapChain();
}

void MGPURenderBackend::DestroyContext() {
  m_mgpu_test_dynamic_buffer.reset(); // TODO: fix this, meh

  DestroySwapChain();
  mgpuCommandListDestroy(m_mgpu_command_list);
  mgpuDeviceDestroy(m_mgpu_device);
}

RenderGeometry* MGPURenderBackend::CreateRenderGeometry(RenderGeometryLayout layout, size_t number_of_vertices, size_t number_of_indices) {
  ZEPHYR_PANIC("unimplemented");
}

void MGPURenderBackend::UpdateRenderGeometryIndices(RenderGeometry* render_geometry, std::span<const u8> data) {
  ZEPHYR_PANIC("unimplemented");
}

void MGPURenderBackend::UpdateRenderGeometryVertices(RenderGeometry* render_geometry, std::span<const u8> data) {
  ZEPHYR_PANIC("unimplemented");
}

void MGPURenderBackend::UpdateRenderGeometryAABB(RenderGeometry* render_geometry, const Box3& aabb) {
  ZEPHYR_PANIC("unimplemented");
}

void MGPURenderBackend::DestroyRenderGeometry(RenderGeometry* render_geometry) {
  ZEPHYR_PANIC("unimplemented");
}

RenderTexture* MGPURenderBackend::CreateRenderTexture(u32 width, u32 height) {
  ZEPHYR_PANIC("unimplemented");
}

void MGPURenderBackend::UpdateRenderTextureData(RenderTexture* render_texture, std::span<const u8> data) {
  ZEPHYR_PANIC("unimplemented");
}

void MGPURenderBackend::DestroyRenderTexture(RenderTexture* render_texture) {
  ZEPHYR_PANIC("unimplemented");
}

void MGPURenderBackend::Render(const RenderCamera& render_camera, const eastl::hash_map<RenderBundleKey, std::vector<RenderBundleItem>>& render_bundles) {
}

void MGPURenderBackend::SwapBuffers() {
  // TODO: since SwapBuffers() happens at the end of the frame, do first texture acquire outside of this

  u32 texture_index{};
  MGPUResult acquire_result = mgpuSwapChainAcquireNextTexture(m_mgpu_swap_chain, &texture_index);
  if(acquire_result == MGPU_SWAP_CHAIN_SUBOPTIMAL) {
    CreateSwapChain();
    MGPU_CHECK(mgpuSwapChainAcquireNextTexture(m_mgpu_swap_chain, &texture_index));
  } else {
    MGPU_CHECK(acquire_result);
  }

  {
    MGPU_CHECK(mgpuCommandListClear(m_mgpu_command_list));

    const MGPURenderPassColorAttachment render_pass_color_attachments[1] {
      {
        .texture_view = m_mgpu_swap_chain_texture_views[texture_index],
        .load_op = MGPU_LOAD_OP_CLEAR,
        .store_op = MGPU_STORE_OP_STORE,
        .clear_color = {.r = 1.f, .g = 1.f, .b = 0.f, .a = 1.f}
      }
    };
    const MGPURenderPassBeginInfo render_pass_info{
      .color_attachment_count = 1u,
      .color_attachments = render_pass_color_attachments,
      .depth_stencil_attachment = nullptr
    };
    MGPURenderCommandEncoder render_cmd_encoder = mgpuCommandListCmdBeginRenderPass(m_mgpu_command_list, &render_pass_info);
    mgpuRenderCommandEncoderClose(render_cmd_encoder);

    MGPU_CHECK(mgpuQueueSubmitCommandList(m_mgpu_queue, m_mgpu_command_list));
  }

  MGPU_CHECK(mgpuSwapChainPresent(m_mgpu_swap_chain));
}

// TODO: move swap chain logic into a dedicated class?

void MGPURenderBackend::CreateSwapChain() {
  u32 surface_format_count{};
  std::vector<MGPUSurfaceFormat> surface_formats{};

  MGPU_CHECK(mgpuPhysicalDeviceEnumerateSurfaceFormats(m_mgpu_physical_device, m_mgpu_surface, &surface_format_count, nullptr));
  surface_formats.resize(surface_format_count);
  MGPU_CHECK(mgpuPhysicalDeviceEnumerateSurfaceFormats(m_mgpu_physical_device, m_mgpu_surface, &surface_format_count, surface_formats.data()));

  bool got_required_surface_format = false;

  for(const MGPUSurfaceFormat& surface_format : surface_formats) {
    if(surface_format.format == MGPU_TEXTURE_FORMAT_B8G8R8A8_SRGB && surface_format.color_space == MGPU_COLOR_SPACE_SRGB_NONLINEAR) {
      got_required_surface_format = true;
    }
  }

  if(!got_required_surface_format) {
    ZEPHYR_PANIC("Failed to find a suitable surface format");
  }

  uint32_t present_modes_count{};
  std::vector<MGPUPresentMode> present_modes{};

  MGPU_CHECK(mgpuPhysicalDeviceEnumerateSurfacePresentModes(m_mgpu_physical_device, m_mgpu_surface, &present_modes_count, nullptr));
  present_modes.resize(present_modes_count);
  MGPU_CHECK(mgpuPhysicalDeviceEnumerateSurfacePresentModes(m_mgpu_physical_device, m_mgpu_surface, &present_modes_count, present_modes.data()));

  MGPUSurfaceCapabilities surface_capabilities{};
  MGPU_CHECK(mgpuPhysicalDeviceGetSurfaceCapabilities(m_mgpu_physical_device, m_mgpu_surface, &surface_capabilities));

  const MGPUSwapChainCreateInfo swap_chain_create_info{
    .surface = m_mgpu_surface,
    .format = MGPU_TEXTURE_FORMAT_B8G8R8A8_SRGB,
    .color_space = MGPU_COLOR_SPACE_SRGB_NONLINEAR,
    .present_mode = MGPU_PRESENT_MODE_FIFO,
    .usage = MGPU_TEXTURE_USAGE_RENDER_ATTACHMENT,
    .extent = surface_capabilities.current_extent,
    .min_texture_count = 2u,
  };
  MGPUSwapChain mgpu_new_swap_chain{};
  MGPU_CHECK(mgpuDeviceCreateSwapChain(m_mgpu_device, &swap_chain_create_info, &mgpu_new_swap_chain));
  DestroySwapChain();
  m_mgpu_swap_chain = mgpu_new_swap_chain;

  u32 texture_count{};
  std::vector<MGPUTexture> mgpu_swap_chain_textures{};
  MGPU_CHECK(mgpuSwapChainEnumerateTextures(m_mgpu_swap_chain, &texture_count, nullptr));
  mgpu_swap_chain_textures.resize(texture_count);
  MGPU_CHECK(mgpuSwapChainEnumerateTextures(m_mgpu_swap_chain, &texture_count, mgpu_swap_chain_textures.data()));

  for(MGPUTexture texture : mgpu_swap_chain_textures) {
    const MGPUTextureViewCreateInfo texture_view_create_info{
      .type = MGPU_TEXTURE_VIEW_TYPE_2D,
      .format = MGPU_TEXTURE_FORMAT_B8G8R8A8_SRGB,
      .aspect = MGPU_TEXTURE_ASPECT_COLOR,
      .base_mip = 0u,
      .mip_count = 1u,
      .base_array_layer = 0u,
      .array_layer_count = 1u
    };

    MGPUTextureView texture_view{};
    MGPU_CHECK(mgpuTextureCreateView(texture, &texture_view_create_info, &texture_view));
    m_mgpu_swap_chain_texture_views.push_back(texture_view);
  }

  const MGPUTextureCreateInfo depth_texture_create_info{
    .format = MGPU_TEXTURE_FORMAT_DEPTH_F32,
    .type = MGPU_TEXTURE_TYPE_2D,
    .extent = {
      .width = surface_capabilities.current_extent.width,
      .height = surface_capabilities.current_extent.height,
      .depth = 1u
    },
    .mip_count = 1u,
    .array_layer_count = 1u,
    .usage = MGPU_TEXTURE_USAGE_RENDER_ATTACHMENT
  };
  MGPU_CHECK(mgpuDeviceCreateTexture(m_mgpu_device, &depth_texture_create_info, &m_mgpu_depth_texture));

  const MGPUTextureViewCreateInfo depth_texture_view_create_info{
    .type = MGPU_TEXTURE_VIEW_TYPE_2D,
    .format = MGPU_TEXTURE_FORMAT_DEPTH_F32,
    .aspect = MGPU_TEXTURE_ASPECT_DEPTH,
    .base_mip = 0u,
    .mip_count = 1u,
    .base_array_layer = 0u,
    .array_layer_count = 1u
  };
  MGPU_CHECK(mgpuTextureCreateView(m_mgpu_depth_texture, &depth_texture_view_create_info, &m_mgpu_depth_texture_view));

  // m_aspect_ratio = (f32)surface_capabilities.current_extent.width / (f32)surface_capabilities.current_extent.height;
}

void MGPURenderBackend::DestroySwapChain() {
  if(!m_mgpu_swap_chain) {
    return;
  }

  mgpuTextureViewDestroy(m_mgpu_depth_texture_view);
  mgpuTextureDestroy(m_mgpu_depth_texture);

  for(MGPUTextureView texture_view : m_mgpu_swap_chain_texture_views) mgpuTextureViewDestroy(texture_view);
  m_mgpu_swap_chain_texture_views.clear();

  mgpuSwapChainDestroy(m_mgpu_swap_chain);
}

} // namespace zephyr

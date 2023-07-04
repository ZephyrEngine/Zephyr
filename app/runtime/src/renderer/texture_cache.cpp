

#include <zephyr/renderer/texture/texture_2D.hpp>
#include <zephyr/renderer/texture/texture_cube.hpp>

#include "texture_cache.hpp"

namespace zephyr {

  TextureCache::TextureCache(
    std::shared_ptr<RenderDevice> render_device,
    std::shared_ptr<ResourceUploader> resource_uploader
  )   : m_render_device{std::move(render_device)}, m_resource_uploader{std::move(resource_uploader)} {
    RegisterAllFormats();
  }

  Texture* TextureCache::GetDeviceTexture(const TextureResource* texture_resource) {
    Entry& entry = m_cache[texture_resource];

    const bool have_device_texture = (bool)entry.device_texture;

    if(!have_device_texture) {
      if(typeid(*texture_resource) == typeid(Texture2D)) {
        auto texture_2D = (const Texture2D*)texture_resource;

        entry.device_texture = m_render_device->CreateTexture2D(
          texture_2D->GetWidth(),
          texture_2D->GetHeight(),
          GetDeviceTextureFormat(texture_resource),
          Texture::Usage::CopySrc | Texture::Usage::CopyDst | Texture::Usage::Sampled,
          GetNumberOfMips(texture_2D->GetWidth(), texture_2D->GetHeight())
        );
      } else if(typeid(*texture_resource) == typeid(TextureCube)) {
        auto texture_cube = (const TextureCube*)texture_resource;

        entry.device_texture = m_render_device->CreateTextureCube(
          texture_cube->GetWidth(),
          texture_cube->GetHeight(),
          GetDeviceTextureFormat(texture_resource),
          Texture::Usage::CopySrc | Texture::Usage::CopyDst | Texture::Usage::Sampled,
          GetNumberOfMips(texture_cube->GetWidth(), texture_cube->GetHeight())
        );
      } else {
        ZEPHYR_PANIC("Unrecognized texture resource sub-class");
      }
    }

    if(!have_device_texture || entry.current_version != texture_resource->CurrentVersion()) {
      CommandBuffer* const command_buffer = m_resource_uploader->GetCurrentCommandBuffer();

      Texture* device_texture = entry.device_texture.get();

      command_buffer->Barrier(
        device_texture,
        PipelineStage::TopOfPipe, PipelineStage::Transfer,
        Access::None, Access::TransferWrite,
        Texture::Layout::Undefined, Texture::Layout::CopyDst,
        0u, device_texture->GetMipCount()
      );


      Buffer* const staging_buffer = m_resource_uploader->GetCurrentStagingBuffer();
      const size_t staging_buffer_offset = m_resource_uploader->AllocateStagingMemory(texture_resource->Size());

      std::memcpy((u8*)staging_buffer->Data() + staging_buffer_offset, texture_resource->Data(), texture_resource->Size());

      command_buffer->CopyBufferToTexture(staging_buffer, staging_buffer_offset, device_texture, Texture::Layout::CopyDst, 0u);

      GenerateMipMaps(command_buffer, device_texture);

      entry.current_version = texture_resource->CurrentVersion();
    }

    return entry.device_texture.get();
  }

  void TextureCache::GenerateMipMaps(CommandBuffer* command_buffer, Texture* device_texture) {
    const u32 mip_count = device_texture->GetMipCount();

    int src_width = (int)device_texture->GetWidth();
    int src_height = (int)device_texture->GetHeight();

    for(u32 mip_level = 1u; mip_level < mip_count; mip_level++) {
      const int dst_width = src_width / 2;
      const int dst_height = src_height / 2;

      command_buffer->Barrier(
        device_texture,
        PipelineStage::Transfer, PipelineStage::Transfer,
        Access::TransferWrite, Access::TransferRead,
        Texture::Layout::CopyDst, Texture::Layout::CopySrc,
        mip_level - 1u, 1u
      );

      command_buffer->BlitTexture2D(
        device_texture,
        device_texture,
        {0, 0, src_width, src_height},
        {0, 0, dst_width, dst_height},
        Texture::Layout::CopySrc,
        Texture::Layout::CopyDst,
        mip_level - 1u,
        mip_level,
        Sampler::FilterMode::Linear
      );

      src_width = dst_width;
      src_height = dst_height;
    }

    // @todo: think of a better place to do the image layout transitions.
    command_buffer->Barrier(
      device_texture,
      PipelineStage::Transfer, PipelineStage::VertexShader,
      Access::TransferRead, Access::ShaderRead,
      Texture::Layout::CopySrc, Texture::Layout::ShaderReadOnly,
      0u, mip_count - 1u
    );
    command_buffer->Barrier(
      device_texture,
      PipelineStage::Transfer, PipelineStage::VertexShader,
      Access::TransferWrite, Access::ShaderRead,
      Texture::Layout::CopyDst, Texture::Layout::ShaderReadOnly,
      mip_count - 1u, 1u
    );
  }

  void TextureCache::RegisterFormat(
    TextureResource::Format format,
    TextureResource::DataType data_type,
    TextureResource::ColorSpace color_space,
    Texture::Format device_format
  ) {
    m_format_table[CalculateFormatKey(format, data_type, color_space)] = device_format;
  }

  Texture::Format TextureCache::GetDeviceTextureFormat(const TextureResource* texture_resource) {
    const auto format = texture_resource->GetFormat();
    const auto data_type = texture_resource->GetDataType();
    const auto color_space = texture_resource->GetColorSpace();

    const auto match = m_format_table.find(CalculateFormatKey(format, data_type, color_space));

    if(match == m_format_table.end()) {
      ZEPHYR_PANIC(
        "Failed to map combination of format={}, data_type={} and color_space={} to device texture format",
        (int)format, (int)data_type, (int)color_space
      );
    }

    return match->second;
  }

  void TextureCache::RegisterAllFormats() {
    using TR = TextureResource;

    RegisterFormat(TR::Format::RGBA, TR::DataType::UnsignedByte, TR::ColorSpace::Linear, Texture::Format::R8G8B8A8_UNORM);
    RegisterFormat(TR::Format::RGBA, TR::DataType::UnsignedByte, TR::ColorSpace::SRGB, Texture::Format::R8G8B8A8_SRGB);
  }

  u32 TextureCache::CalculateFormatKey(
    TextureResource::Format format,
    TextureResource::DataType data_type,
    TextureResource::ColorSpace color_space
  ) {
    static_assert((size_t)TextureResource::Format::Limit <= 256);
    static_assert((size_t)TextureResource::DataType::Limit <= 256);
    static_assert((size_t)TextureResource::ColorSpace::Limit <= 256);

    return (u8)format | (u8)data_type << 8 | (u8)color_space << 16;
  }

} // namespace zephyr
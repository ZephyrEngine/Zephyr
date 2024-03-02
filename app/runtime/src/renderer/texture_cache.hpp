
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/renderer/texture/texture_resource.hpp>
#include <cmath>
#include <unordered_map>

#include "resource_uploader.hpp"

namespace zephyr {

  class TextureCache {
    public:
      TextureCache(
        std::shared_ptr<RenderDevice> render_device,
        std::shared_ptr<ResourceUploader> resource_uploader
      );

      Texture* GetDeviceTexture(const TextureResource* texture_resource);

    private:
      struct Entry {
        u64 current_version;
        std::unique_ptr<Texture> device_texture;
      };

      void GenerateMipMaps(CommandBuffer* command_buffer, Texture* device_texture);

      void RegisterFormat(
        TextureResource::Format format,
        TextureResource::DataType data_type,
        TextureResource::ColorSpace color_space,
        Texture::Format device_format
      );

      Texture::Format GetDeviceTextureFormat(const TextureResource* texture_resource);

      void RegisterAllFormats();

      static u32 CalculateFormatKey(
        TextureResource::Format format,
        TextureResource::DataType data_type,
        TextureResource::ColorSpace color_space
      );

      static u32 GetNumberOfMips(uint width, uint height, uint depth = std::numeric_limits<uint>::max()) {
        return (u32)std::ceil(std::log2(std::min(width, std::min(height, depth))));
      }

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<ResourceUploader> m_resource_uploader;
      std::unordered_map<u64, Entry> m_cache;
      std::unordered_map<u32, Texture::Format> m_format_table;
  };

} // namespace zephyr

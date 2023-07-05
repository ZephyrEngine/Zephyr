
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/gpu/sampler.hpp>
#include <zephyr/renderer/texture/sampler_resource.hpp>
#include <unordered_map>

namespace zephyr {

  class SamplerCache {
    public:
      SamplerCache(std::shared_ptr<RenderDevice> render_device);

      Sampler* GetDeviceSampler(const SamplerResource* sampler_resource);

    private:
      struct Entry {
        u64 current_version{};
        std::unique_ptr<Sampler> device_sampler;
      };

      std::shared_ptr<RenderDevice> m_render_device;
      std::unordered_map<const SamplerResource*, Entry> m_cache;
  };

} // namespace zephyr
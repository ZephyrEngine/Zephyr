
#include "sampler_cache.hpp"

namespace zephyr {

  SamplerCache::SamplerCache(std::shared_ptr<RenderDevice> render_device)
      : m_render_device{std::move(render_device)} {
  }

  Sampler* SamplerCache::GetDeviceSampler(const zephyr::SamplerResource* sampler_resource) {
    Entry& entry = m_cache[sampler_resource->GetUID()];

    if(!entry.device_sampler || entry.current_version != sampler_resource->CurrentVersion()) {
      entry.device_sampler = m_render_device->CreateSampler({
        .address_mode_u = sampler_resource->GetWrapU(),
        .address_mode_v = sampler_resource->GetWrapV(),
        .mag_filter = sampler_resource->GetMagFilter(),
        .min_filter = sampler_resource->GetMinFilter(),
        .mip_filter = Sampler::FilterMode::Linear
      });

      entry.current_version = sampler_resource->CurrentVersion();
    }

    return entry.device_sampler.get();
  }

} // namespace zephyr

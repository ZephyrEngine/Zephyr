
#pragma once

#include <zephyr/gpu/sampler.hpp>
#include <zephyr/renderer/resource.hpp>

namespace zephyr {

  class SamplerResource : public Resource {
    public:
      [[nodiscard]] Sampler::AddressMode GetWrapU() const {
        return m_address_mode_u;
      }

      void SetWrapU(Sampler::AddressMode wrap) {
        m_address_mode_u = wrap;
      }

      [[nodiscard]] Sampler::AddressMode GetWrapV() const {
        return m_address_mode_u;
      }

      void SetWrapV(Sampler::AddressMode wrap) {
        m_address_mode_v = wrap;
      }

      [[nodiscard]] Sampler::FilterMode GetMagFilter() const {
        return m_mag_filter;
      }

      void SetMagFilter(Sampler::FilterMode filter) {
        m_mag_filter = filter;
      }

      [[nodiscard]] Sampler::FilterMode GetMinFilter() const {
        return m_min_filter;
      }

      void SetMinFilter(Sampler::FilterMode filter) {
        m_min_filter = filter;
      }

    private:
      Sampler::AddressMode m_address_mode_u{Sampler::AddressMode::Repeat};
      Sampler::AddressMode m_address_mode_v{Sampler::AddressMode::Repeat};
      Sampler::FilterMode m_mag_filter{Sampler::FilterMode::Linear};
      Sampler::FilterMode m_min_filter{Sampler::FilterMode::Linear};
  };

} // namespace zephyr

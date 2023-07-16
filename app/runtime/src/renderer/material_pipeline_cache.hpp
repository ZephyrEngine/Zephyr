
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/integer.hpp>
#include <array>
#include <functional>
#include <limits>
#include <type_traits>
#include <unordered_map>

#include "material.hpp"

// @todo: put this into a common header or something
struct pair_hash {
  template<typename T1, typename T2>
  auto operator()(std::pair<T1, T2> const& pair) const noexcept -> size_t {
    auto hash0 = std::hash<T1>{}(pair.first);
    auto hash1 = std::hash<T2>{}(pair.second);

    return hash1 ^ (0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
  }
};

namespace zephyr {

  class MaterialPipelineCache {
    public:

      explicit MaterialPipelineCache(std::shared_ptr<RenderDevice> render_device);

      // @todo: consider the geometry of the mesh we are rendering.
      // @todo: return a reference instead of a pointer - do the same for the sampler, texture and buffer caches
      GraphicsPipeline* GetGraphicsPipeline(const MaterialShader& material_shader);

    private:
      /*struct Entry {
        static constexpr size_t k_max_technique_count = 256;

        static_assert(
          std::is_unsigned_v<std::underlying_type_t<Technique>> &&
          std::numeric_limits<std::underlying_type_t<Technique>>::max() < k_max_technique_count
        );

        struct TechniqueEntry {
          std::unique_ptr<GraphicsPipelineBuilder> pipeline_builder;
          std::unordered_map<ShaderVariantKey, std::unique_ptr<GraphicsPipeline>> pipelines;
        };

        std::array<TechniqueEntry, k_max_technique_count> techniques;
      };*/

      struct CacheKey {
        const MaterialShader* shader;
        Technique technique;
        u64 variant;
      };

      struct CacheKeyHash {
        size_t operator()(const CacheKey& cache_key) const noexcept {
          // @note: assumes the top 8-bit of the 64-bit address space are unused
          auto hash0 = std::hash<u64>{}((u64)cache_key.shader | (u64)cache_key.technique << 56);
          auto hash1 = std::hash<u64>{}(cache_key.variant);

          return hash1 ^ (0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
        }
      };

      struct Entry {
      };

      std::shared_ptr<RenderDevice> m_render_device;
      std::unordered_map<CacheKey, Entry, CacheKeyHash> m_cache;
      //std::unordered_map<const MaterialShader*, Entry> m_cache;
  };

} // namespace zephyr

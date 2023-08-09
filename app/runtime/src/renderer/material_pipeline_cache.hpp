
#pragma once

#include <zephyr/gpu/render_device.hpp>
#include <zephyr/renderer/material/material.hpp>
#include <zephyr/renderer/mesh.hpp>
#include <zephyr/integer.hpp>
#include <array>
#include <functional>
#include <limits>
#include <type_traits>
#include <unordered_map>

#include "material.hpp"

namespace zephyr {

  class MaterialPipelineCache {
    public:
      explicit MaterialPipelineCache(std::shared_ptr<RenderDevice> render_device);

      void RegisterTechnique(Technique technique, std::shared_ptr<RenderPass> render_pass);

      // @todo: use references (and just buffer, texture and sampler caches accordingly)
      // @todo: get rid of the render pass parameter
      GraphicsPipeline* GetGraphicsPipeline(Technique technique, const Material* material, const Mesh3D* mesh);

    private:
      struct Key {
        const MaterialShader* shader;
        u64 variant_key;
        Technique technique;

        bool operator==(const Key& other) const {
          return shader == other.shader &&
                 variant_key == other.variant_key &&
                 technique == other.technique;
        }
      };

      struct KeyHash {
        size_t operator()(const Key& key) const noexcept {
          const size_t hash0 = std::hash<u64>{}((u64)key.shader << 8 | (u64)key.technique);
          const size_t hash1 = std::hash<u64>{}(key.variant_key);

          return hash1 ^ (0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
        }
      };

      struct Entry {
        // @todo: do not keep the pipeline builder alive, ideally
        std::unique_ptr<GraphicsPipelineBuilder> pipeline_builder;
        std::unique_ptr<GraphicsPipeline> pipeline;
      };

      void CreateBindGroupLayout();

      u64 BuildVariantKey(Technique technique, const Material* material, const Mesh3D* mesh);

      std::shared_ptr<RenderDevice> m_render_device;
      std::shared_ptr<BindGroupLayout> m_bind_group_layout;
      std::unordered_map<const Key, Entry, KeyHash> m_cache;
      std::array<std::shared_ptr<RenderPass>, 256> m_technique_render_pass;

      static_assert(std::is_same_v<std::underlying_type_t<Technique>, u8>);
  };

} // namespace zephyr

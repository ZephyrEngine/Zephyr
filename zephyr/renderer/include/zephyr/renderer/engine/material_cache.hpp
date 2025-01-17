
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/engine/texture_cache.hpp>
#include <zephyr/renderer/resource/material.hpp>
#include <zephyr/integer.hpp>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>

namespace zephyr {

class MaterialCache {
  public:
    MaterialCache(std::shared_ptr<RenderBackend> render_backend, TextureCache& texture_cache);

    // Game Thread API:
    void IncrementMaterialRefCount(const Material* material);
    void DecrementMaterialRefCount(const Material* material);

  private:
    struct MaterialState {
      size_t ref_count{};
    };

    std::shared_ptr<RenderBackend> m_render_backend;
    TextureCache& m_texture_cache;
    eastl::hash_set<const Material*> m_used_material_set{};
    eastl::hash_map<const Material*, MaterialState> m_material_state_table{};
};

} // namespace zephyr

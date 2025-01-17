
#include <zephyr/renderer/engine/material_cache.hpp>

namespace zephyr {

MaterialCache::MaterialCache(std::shared_ptr<RenderBackend> render_backend, TextureCache& texture_cache)
    : m_render_backend{std::move(render_backend)}
    , m_texture_cache{texture_cache} {
}

void MaterialCache::IncrementMaterialRefCount(const Material* material) {
  if(++m_material_state_table[material].ref_count == 1u) {
    m_used_material_set.insert(material);

    // TODO(fleroviux): in the future we need to deal with changing texture assignments
    if(material->m_diffuse_map) {
      m_texture_cache.IncrementTextureRefCount(material->m_diffuse_map.get());
    }
  }
}

void MaterialCache::DecrementMaterialRefCount(const Material* material) {
  if(--m_material_state_table[material].ref_count == 0u) {
    m_used_material_set.erase(material);

    // TODO(fleroviux): in the future we need to deal with changing texture assignments
    if(material->m_diffuse_map) {
      m_texture_cache.DecrementTextureRefCount(material->m_diffuse_map.get());
    }
  }
}

} // namespace zephyr

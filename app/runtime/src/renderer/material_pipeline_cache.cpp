
#include "material_pipeline_cache.hpp"

namespace zephyr {

  MaterialPipelineCache::MaterialPipelineCache(std::shared_ptr<RenderDevice> render_device)
      : m_render_device{std::move(render_device)} {
  }

  GraphicsPipeline* MaterialPipelineCache::GetGraphicsPipeline(const MaterialShader& material_shader) {
    return nullptr;
  }

} // namespace zephyr

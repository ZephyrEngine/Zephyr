
#include <zephyr/panic.hpp>

#include "shader/mesh.vert.h"
#include "shader/mesh.frag.h"
#include "material_pipeline_cache.hpp"

namespace zephyr {

  MaterialPipelineCache::MaterialPipelineCache(std::shared_ptr<RenderDevice> render_device)
      : m_render_device{std::move(render_device)} {
    m_technique_render_pass.fill(nullptr);

    CreateBindGroupLayout();
  }

  void MaterialPipelineCache::RegisterTechnique(Technique technique, std::shared_ptr<RenderPass> render_pass) {
    m_technique_render_pass[(u8)technique] = std::move(render_pass);
  }

  GraphicsPipeline* MaterialPipelineCache::GetGraphicsPipeline(Technique technique, const Material* material, const Mesh3D* mesh) {
    const u64 variant_key = BuildVariantKey(technique, material, mesh);
    const Key key{&material->GetShader(), variant_key, technique};

    auto& entry = m_cache[key];

    // @todo: support some level of versioning for the material shader

    if(!entry.pipeline) {
      auto pipeline_builder = m_render_device->CreateGraphicsPipelineBuilder();

      std::shared_ptr<ShaderModule> vert_shader = m_render_device->CreateShaderModule(mesh_vert, sizeof(mesh_vert));
      std::shared_ptr<ShaderModule> frag_shader = m_render_device->CreateShaderModule(mesh_frag, sizeof(mesh_frag));

      std::shared_ptr<RenderPass> render_pass = m_technique_render_pass[(u8)technique];

      if(!render_pass) {
        ZEPHYR_PANIC("No render pass has been registered for technique: {}", (u8)technique);
      }

      pipeline_builder->SetDynamicViewportEnable(true);
      pipeline_builder->SetShaderModule(ShaderStage::Vertex, vert_shader);
      pipeline_builder->SetShaderModule(ShaderStage::Fragment, frag_shader);
      pipeline_builder->SetRenderPass(render_pass);
      pipeline_builder->SetDepthTestEnable(true);
      pipeline_builder->SetDepthWriteEnable(true);
      pipeline_builder->SetDepthCompareOp(CompareOp::LessOrEqual);
      pipeline_builder->AddVertexInputBinding(0, sizeof(float) * 9);
      pipeline_builder->AddVertexInputAttribute(0, 0, 0, VertexDataType::Float32, 3, false);
      pipeline_builder->AddVertexInputAttribute(1, 0, sizeof(float) * 3, VertexDataType::Float32, 4, false);
      pipeline_builder->AddVertexInputAttribute(2, 0, sizeof(float) * 7, VertexDataType::Float32, 2, false);
      pipeline_builder->SetPipelineLayout(m_render_device->CreatePipelineLayout({{m_bind_group_layout.get()}})); // @todo: do not keep recreating the pipeline layout

      entry.pipeline = pipeline_builder->Build();
      entry.pipeline_builder = std::move(pipeline_builder);
    }

    return entry.pipeline.get();
  }

  u64 MaterialPipelineCache::BuildVariantKey(Technique technique, const Material* material, const Mesh3D* mesh) {
    u64 key = (u64)mesh->GetLayoutKey() << 32;

    const std::span<const std::shared_ptr<Texture2D>> textures = material->GetTextures();

    // @todo: think of a better method to enforce this upper limit.
    if(textures.size() >= 32u) {
      ZEPHYR_PANIC("Material has {} texture slots, however the allowed limit is 32 slots.", textures.size());
    }

    for(size_t i = 0u; i < textures.size(); i++) {
      if((bool)textures[i]) key |= 1ul << i;
    }

    // @todo: force some key bits to zero based on the render technique used - not all techniques will use all
    // textures for example and reducing the number variants will reduce the number of pipeline switches
    // and will reduce shader compilation overhead.
    // Maybe adding some extra key bits depending on the technique will also become necessary eventually.

    return key;
  }

  void MaterialPipelineCache::CreateBindGroupLayout() {
    std::array<BindGroupLayout::Entry, 33> bindings;

    for(u32 binding = 0; binding < 32u; binding++) {
      bindings[binding] = {
        .binding = binding,
        .type = BindingType::ImageWithSampler,
        .stages = ShaderStage::All
      };
    }

    bindings[32] = {
      .binding = 32u,
      .type = BindingType::UniformBuffer,
      .stages = ShaderStage::All
    };

    m_bind_group_layout = m_render_device->CreateBindGroupLayout(bindings);
  }

} // namespace zephyr

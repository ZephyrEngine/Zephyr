
#pragma once

#include <zephyr/gpu/pipeline_layout.hpp>
#include <zephyr/gpu/enums.hpp>
#include <zephyr/gpu/render_pass.hpp>
#include <zephyr/gpu/shader_module.hpp>
#include <zephyr/integer.hpp>
#include <memory>

namespace zephyr {

  class GraphicsPipeline {
    public:
      virtual ~GraphicsPipeline() = default;

      virtual void* Handle() = 0;
      virtual PipelineLayout* GetLayout() = 0;
  };

  class GraphicsPipelineBuilder {
    public:
      virtual ~GraphicsPipelineBuilder() = default;

      virtual void SetViewport(int x, int y, int width, int height) = 0;
      virtual void SetScissor(int x, int y, int width, int height) = 0;
      virtual void SetShaderModule(ShaderStage stage, std::shared_ptr<ShaderModule> shader_module) = 0;
      virtual void SetPipelineLayout(std::shared_ptr<PipelineLayout> layout) = 0;
      virtual void SetRenderPass(std::shared_ptr<RenderPass> render_pass) = 0;
      virtual void SetRasterizerDiscardEnable(bool enable) = 0;
      virtual void SetPolygonMode(PolygonMode mode) = 0;
      virtual void SetPolygonCull(PolygonFace face) = 0;
      virtual void SetLineWidth(float width) = 0;
      virtual void SetDepthTestEnable(bool enable) = 0;
      virtual void SetDepthWriteEnable(bool enable) = 0;
      virtual void SetDepthCompareOp(CompareOp compare_op) = 0;
      virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;
      virtual void SetPrimitiveRestartEnable(bool enable) = 0;
      virtual void SetBlendEnable(size_t color_attachment, bool enable) = 0;
      virtual void SetSrcColorBlendFactor(size_t color_attachment, BlendFactor blend_factor) = 0;
      virtual void SetSrcAlphaBlendFactor(size_t color_attachment, BlendFactor blend_factor) = 0;
      virtual void SetDstColorBlendFactor(size_t color_attachment, BlendFactor blend_factor) = 0;
      virtual void SetDstAlphaBlendFactor(size_t color_attachment, BlendFactor blend_factor) = 0;
      virtual void SetColorBlendOp(size_t color_attachment, BlendOp blend_op) = 0;
      virtual void SetAlphaBlendOp(size_t color_attachment, BlendOp blend_op) = 0;
      virtual void SetColorWriteMask(size_t color_attachment, ColorComponent components) = 0;
      virtual void SetBlendConstants(float r, float g, float b, float a) = 0;
      virtual void SetPatchControlPointCount(int patch_control_points) = 0;

      virtual void ResetVertexInput() = 0;
      virtual void AddVertexInputBinding(u32 binding, u32 stride, VertexInputRate input_rate = VertexInputRate::Vertex) = 0;
      virtual void AddVertexInputAttribute(
        u32 location,
        u32 binding,
        u32 offset,
        VertexDataType data_type,
        int components,
        bool normalized
      ) = 0;

      virtual void SetDynamicViewportEnable(bool enable) = 0;
      virtual void SetDynamicScissorEnable(bool enable) = 0;

      virtual auto Build() -> std::unique_ptr<GraphicsPipeline> = 0;
  };

} // namespace zephyr

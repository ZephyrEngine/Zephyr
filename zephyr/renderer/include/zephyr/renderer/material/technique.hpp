
#pragma once

#include <zephyr/math/vector.hpp>
#include <zephyr/gpu/enums.hpp>
#include <zephyr/integer.hpp>
#include <optional>

namespace zephyr {

  enum class Technique : u8 {
    EarlyZ,
    Shadow,
    GBuffer,
    ForwardOpaque,
    ForwardTransparent
  };

  struct TechniqueBlendState {
    struct {
      BlendFactor color_blend_factor;
      BlendFactor alpha_blend_factor;
    } src, dst;

    BlendOp color_blend_op;
    BlendOp alpha_blend_op;
    Vector4 constant_color;
  };

  struct TechniqueDepthState {
    bool enable_test;
    bool enable_write;
    CompareOp compare_op;
  };

  struct TechniquePass {
    std::string vert_shader_path;
    std::string frag_shader_path;
    std::optional<TechniqueBlendState> blend_state;
    std::optional<TechniqueDepthState> depth_state;
  };

} // namespace zephyr

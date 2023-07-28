
#pragma once

#include <zephyr/renderer/material/material.hpp>
#include <array>

namespace zephyr {

  class PBRMaterialShader final : public MaterialShader {
    public:
      PBRMaterialShader() {
        GLSLVariableList parameters;
        parameters.Add<f32>("roughness");
        parameters.Add<f32>("metalness");
        m_buffer_layout = STD430BufferLayout{parameters};
      }

      [[nodiscard]] const STD430BufferLayout& GetParameterBufferLayout() const override {
        return m_buffer_layout;
      }

      [[nodiscard]] std::span<const std::string> GetTextureSlots() const override {
        return k_texture_slots;
      }

      [[nodiscard]] std::optional<std::reference_wrapper<const TechniquePass>> GetTechniquePass(Technique technique) const override {
        switch(technique) {
          case Technique::Forward: {
            static const TechniquePass pass{"shader/gbuffer.vert.glsl", "shader/gbuffer.frag.glsl"};
            return {pass};
          }
          default: {
            return std::nullopt;
          }
        }
      }

    private:
      static constexpr std::array<std::string, 3> k_texture_slots{
        "u_diffuse_map",
        "u_roughness_map",
        "u_metalness_map"
      };

      STD430BufferLayout m_buffer_layout;
  };

} // namespace zephyr

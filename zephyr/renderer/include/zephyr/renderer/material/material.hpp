
#pragma once

#include <zephyr/renderer/buffer/uniform_buffer.hpp>
#include <zephyr/renderer/glsl/std430_buffer_layout.hpp>
#include <zephyr/renderer/material/technique.hpp>
#include <zephyr/renderer/texture/texture_2D.hpp>
#include <zephyr/panic.hpp>
#include <span>
#include <optional>
#include <memory>
#include <vector>

namespace zephyr {

  class MaterialShader {
    public:
      [[nodiscard]] virtual const STD430BufferLayout& GetParameterBufferLayout() const = 0;
      [[nodiscard]] virtual std::span<const std::string> GetTextureSlots() const = 0;
      [[nodiscard]] virtual std::optional<std::reference_wrapper<const TechniquePass>> GetTechniquePass(Technique technique) const = 0;
  };

  class Material {
    public:
      explicit Material(std::shared_ptr<MaterialShader> shader) : m_shader{std::move(shader)} {
        m_ubo = std::make_unique<UniformBuffer>(m_shader->GetParameterBufferLayout().Size());
        m_textures.resize(m_shader->GetTextureSlots().size());
      }

      [[nodiscard]] const MaterialShader& GetShader() const {
        return *m_shader;
      }

      [[nodiscard]] const UniformBuffer& GetUniformBuffer() const {
        return *m_ubo;
      }

      [[nodiscard]] std::span<const std::shared_ptr<Texture2D>> GetTextures() const {
        return m_textures;
      }

      template<typename T>
      [[nodiscard]] const T& GetParameter(const std::string& name) const {
        return *const_cast<Material*>(this)->GetPointerToVariable<const T>(name);
      }

      template<typename T>
      void SetParameter(const std::string& name, const T& value) {
        *GetPointerToVariable<T>(name) = value;
        m_ubo->MarkAsDirty();
      }

      [[nodiscard]] const std::shared_ptr<Texture2D>& GetTexture(const std::string& name) const {
        return m_textures[FindTextureSlotByName(name)];
      }

      void SetTexture(const std::string& name, std::shared_ptr<Texture2D> texture) {
        m_textures[FindTextureSlotByName(name)] = std::move(texture);
      }

    private:
      [[nodiscard]] size_t FindTextureSlotByName(const std::string& name) const {
        const auto slots = GetShader().GetTextureSlots();

        const ptrdiff_t position = std::find(slots.begin(), slots.end(), name) - slots.begin();

        if(position >= slots.size()) {
          ZEPHYR_PANIC("Could not find a texture slot named '{}'", name);
        }

        return (size_t)position;
      }

      template<typename T>
      T* GetPointerToVariable(const std::string& name) {
        const STD430BufferLayout::Variable& variable = m_shader->GetParameterBufferLayout().GetVariable(name);

        if(variable.type != GLSLType::FromCPPType<T>()) {
          ZEPHYR_PANIC("GLSL variable type did not match with C++ type");
        }

        return (T*)(m_ubo->Data<u8>() + variable.buffer_offset);
      }

      std::shared_ptr<MaterialShader> m_shader;
      std::unique_ptr<UniformBuffer> m_ubo;
      std::vector<std::shared_ptr<Texture2D>> m_textures;
  };

} // namespace zephyr

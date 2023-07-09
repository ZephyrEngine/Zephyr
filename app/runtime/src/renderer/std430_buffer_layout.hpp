
#pragma once

#include <algorithm>
#include <zephyr/math/matrix4.hpp>
#include <zephyr/math/vector.hpp>
#include <zephyr/float.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <string>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace zephyr {

  class GLSLType {
    public:
      enum class ScalarType {
        Bool,
        Sint,
        Uint,
        F32,
        F64
      };

      enum class Grade {
        Scalar,
        Vec2,
        Vec3,
        Vec4,
        Mat4
      };

      GLSLType(ScalarType scalar_type, Grade grade)
        : m_scalar_type{scalar_type}, m_grade{grade} {
      }

      [[nodiscard]] ScalarType GetScalarType() const {
        return m_scalar_type;
      }

      [[nodiscard]] Grade GetGrade() const {
        return m_grade;
      }

      [[nodiscard]] bool operator==(const GLSLType& other) const {
        return m_scalar_type == other.m_scalar_type && m_grade == other.m_grade;
      }

      [[nodiscard]] explicit operator std::string() const;

      template<typename T>
      [[nodiscard]] static GLSLType FromCPPType() {
        using U = std::remove_const<T>::type;

        if constexpr(std::is_same_v<U, bool>) return {GLSLType::ScalarType::Bool, GLSLType::Grade::Scalar};
        if constexpr(std::is_same_v<U,  int>) return {GLSLType::ScalarType::Sint, GLSLType::Grade::Scalar};
        if constexpr(std::is_same_v<U, uint>) return {GLSLType::ScalarType::Uint, GLSLType::Grade::Scalar};
        if constexpr(std::is_same_v<U,  f32>) return {GLSLType::ScalarType::F32,  GLSLType::Grade::Scalar};
        if constexpr(std::is_same_v<U,  f64>) return {GLSLType::ScalarType::F64,  GLSLType::Grade::Scalar};

        if constexpr(std::is_same_v<U, Vector2>) return {GLSLType::ScalarType::F32, GLSLType::Grade::Vec2};
        if constexpr(std::is_same_v<U, Vector3>) return {GLSLType::ScalarType::F32, GLSLType::Grade::Vec3};
        if constexpr(std::is_same_v<U, Vector4>) return {GLSLType::ScalarType::F32, GLSLType::Grade::Vec4};
        if constexpr(std::is_same_v<U, Matrix4>) return {GLSLType::ScalarType::F32, GLSLType::Grade::Mat4};

        ZEPHYR_PANIC("Unsupported C++ type: {}", typeid(T).name());
      }

    private:
      ScalarType m_scalar_type{};
      Grade m_grade{};
  };

  class STD430BufferLayout {
    public:
      struct Variable {
        GLSLType type;
        std::string name;
        size_t array_size;
        size_t buffer_offset;
        size_t data_size;
        size_t data_alignment;
      };

      [[nodiscard]] std::span<const Variable> GetVariables() const {
        return m_variables;
      }

      [[nodiscard]] size_t Size() const {
        return m_size;
      }

      template<typename T>
      void Add(std::string name) {
        Add<T>(name, 0u);
      }

      template<typename T>
      void Add(std::string name, size_t array_size) {
        if(m_variable_map.find(name) != m_variable_map.end()) {
          ZEPHYR_PANIC("Duplicate variable name in std430 buffer layout: {}", name);
        }

        m_variables.push_back({GLSLType::FromCPPType<T>(), name, array_size});
        m_variable_map[name] = &m_variables.back();
      }

      const Variable& GetVariable(const std::string& name) const {
        if(auto match = m_variable_map.find(name); match != m_variable_map.end()) {
          return *match->second;
        }

        ZEPHYR_PANIC("No variable named '{}' found in std430 buffer layout", name);
      }

      void Build() {
        for(auto& variable : m_variables) {
          const bool is_array = variable.array_size != 0u;

          variable.data_size = GLSLTypeToSize(variable.type, is_array);
          variable.data_alignment = GLSLTypeToAlignment(variable.type, is_array);

          if(is_array) {
            variable.data_size *= variable.array_size;
          }
        }

        std::sort(m_variables.begin(), m_variables.end(), [](const Variable& a, const Variable& b) {
          return a.data_alignment > b.data_alignment;
        });

        size_t buffer_offset = 0u;

        for(auto& variable : m_variables) {
          const size_t remainder = buffer_offset % variable.data_alignment;

          if(remainder != 0u) {
            buffer_offset += variable.data_alignment - remainder;
          }

          variable.buffer_offset = buffer_offset;
          buffer_offset += variable.data_size;
        }

        m_size = buffer_offset;
      }

    private:

      [[nodiscard]] static size_t GLSLTypeToSize(const GLSLType& type, bool is_inside_array) {
        return GetNumberOfComponentsFromGrade(type.GetGrade(), is_inside_array) *
          GetScalarTypeSizeInBytes(type.GetScalarType());
      }

      [[nodiscard]] static size_t GLSLTypeToAlignment(const GLSLType& type, bool is_inside_array) {
        if(type.GetGrade() == GLSLType::Grade::Mat4) {
          return 4u * GetScalarTypeSizeInBytes(type.GetScalarType());
        }
        return GLSLTypeToSize(type, is_inside_array);
      }

      [[nodiscard]] static size_t GetScalarTypeSizeInBytes(GLSLType::ScalarType type) {
        return type == GLSLType::ScalarType::F64 ? 8u : 4u;
      }

      [[nodiscard]] static size_t GetNumberOfComponentsFromGrade(GLSLType::Grade grade, bool is_inside_array) {
        switch(grade) {
          case GLSLType::Grade::Scalar: return 1u;
          case GLSLType::Grade::Vec2:   return 2u;
          case GLSLType::Grade::Vec3:   return is_inside_array ? 3u : 4u;
          case GLSLType::Grade::Vec4:   return 4u;
          case GLSLType::Grade::Mat4:   return 16u;
          default: ZEPHYR_PANIC("Unhandled GLSL type grade: {}", (int)grade);
        }
      }

      std::vector<Variable> m_variables;
      std::unordered_map<std::string, Variable*> m_variable_map;
      std::size_t m_size{};
  };

  inline GLSLType::operator std::string() const {
    if(GetGrade() == Grade::Scalar) {
      switch(GetScalarType()) {
        case ScalarType::Bool: return "bool";
        case ScalarType::Sint: return "int";
        case ScalarType::Uint: return "uint";
        case ScalarType::F32:  return "float";
        case ScalarType::F64:  return "double";
        default: ZEPHYR_PANIC("Unhandled scalar type: {}", (int)GetScalarType())
      }
    } else {
      std::string prefix;

      switch(GetScalarType()) {
        case ScalarType::Bool: prefix = "b"; break;
        case ScalarType::Sint: prefix = "i"; break;
        case ScalarType::Uint: prefix = "u"; break;
        case ScalarType::F32:  break;
        case ScalarType::F64:  prefix = "d"; break;
        default: ZEPHYR_PANIC("Unhandled scalar type: {}", (int)GetScalarType());
      }

      switch(GetGrade()) {
        case Grade::Vec2: return prefix + "vec2";
        case Grade::Vec3: return prefix + "vec3";
        case Grade::Vec4: return prefix + "vec4";
        case Grade::Mat4: return prefix + "mat4";
        default: ZEPHYR_PANIC("Unhandled grade: {}", (int)GetGrade());
      }
    }
  }

} // namespace zephyr

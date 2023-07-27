
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <zephyr/math/vector.hpp>
#include <zephyr/float.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <string>
#include <typeinfo>
#include <type_traits>

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

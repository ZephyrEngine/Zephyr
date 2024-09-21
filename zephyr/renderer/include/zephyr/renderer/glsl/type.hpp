
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <zephyr/math/vector.hpp>
#include <zephyr/float.hpp>
#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <string>
#include <typeinfo>
#include <type_traits>

namespace zephyr::glsl {

class Type {
  public:
    enum class ScalarType {
      Bool,
      I32,
      U32,
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

    Type(ScalarType scalar_type, Grade grade)
      : m_scalar_type{scalar_type}, m_grade{grade} {
    }

    [[nodiscard]] ScalarType GetScalarType() const {
      return m_scalar_type;
    }

    [[nodiscard]] Grade GetGrade() const {
      return m_grade;
    }

    [[nodiscard]] bool operator==(const Type& other) const {
      return m_scalar_type == other.m_scalar_type && m_grade == other.m_grade;
    }

    [[nodiscard]] explicit operator std::string() const;

    template<typename T>
    [[nodiscard]] static Type FromCPPType() {
      using U = std::remove_const<T>::type;

      if constexpr(std::is_same_v<U, bool>) return {Type::ScalarType::Bool, Type::Grade::Scalar};
      if constexpr(std::is_same_v<U,  i32>) return {Type::ScalarType::I32,  Type::Grade::Scalar};
      if constexpr(std::is_same_v<U,  u32>) return {Type::ScalarType::U32,  Type::Grade::Scalar};
      if constexpr(std::is_same_v<U,  f32>) return {Type::ScalarType::F32,  Type::Grade::Scalar};
      if constexpr(std::is_same_v<U,  f64>) return {Type::ScalarType::F64,  Type::Grade::Scalar};

      if constexpr(std::is_same_v<U, Vector2>) return {Type::ScalarType::F32, Type::Grade::Vec2};
      if constexpr(std::is_same_v<U, Vector3>) return {Type::ScalarType::F32, Type::Grade::Vec3};
      if constexpr(std::is_same_v<U, Vector4>) return {Type::ScalarType::F32, Type::Grade::Vec4};
      if constexpr(std::is_same_v<U, Matrix4>) return {Type::ScalarType::F32, Type::Grade::Mat4};

      ZEPHYR_PANIC("Unsupported C++ type: {}", typeid(T).name());
    }

  private:
    ScalarType m_scalar_type{};
    Grade m_grade{};
};

inline Type::operator std::string() const {
  if(GetGrade() == Grade::Scalar) {
    switch(GetScalarType()) {
      case ScalarType::Bool: return "bool";
      case ScalarType::I32:  return "int";
      case ScalarType::U32:  return "uint";
      case ScalarType::F32:  return "float";
      case ScalarType::F64:  return "double";
      default: ZEPHYR_PANIC("Unhandled scalar type: {}", (int)GetScalarType())
    }
  } else {
    std::string prefix;

    switch(GetScalarType()) {
      case ScalarType::Bool: prefix = "b"; break;
      case ScalarType::I32:  prefix = "i"; break;
      case ScalarType::U32:  prefix = "u"; break;
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

} // namespace zephyr::glsl

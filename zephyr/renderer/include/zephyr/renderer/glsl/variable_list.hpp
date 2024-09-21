#pragma once

#include <zephyr/renderer/glsl/type.hpp>

#include <span>
#include <vector>
#include <unordered_set>

namespace zephyr::glsl {

class VariableList {
  public:
    struct Variable {
      Type type;
      std::string name;
      size_t array_size;
    };

    [[nodiscard]] std::span<const Variable> GetVariables() const {
      return m_variables;
    }

    template<typename T>
    void Add(std::string name) {
      Add<T>(name, 0u);
    }

    template<typename T>
    void Add(std::string name, size_t array_size) {
      if(m_variable_set.find(name) != m_variable_set.end()) {
        ZEPHYR_PANIC("Duplicate variable name in GLSL variable list: {}", name);
      }

      m_variables.push_back({Type::FromCPPType<T>(), name, array_size});
      m_variable_set.insert(name);
    }

  private:
    std::vector<Variable> m_variables{};
    std::unordered_set<std::string> m_variable_set{};
};

} // namespace zephyr::glsl

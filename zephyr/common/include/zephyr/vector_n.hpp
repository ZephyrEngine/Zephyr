
#pragma once

#include <cstddef>
#include <utility>
#include <initializer_list>

namespace zephyr {

  template <typename T, std::size_t capacity>
  class Vector_N {
    public:
      using value_type = T;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using reference = T&;
      using const_reference = const T&;
      using pointer = T*;
      using const_pointer = const T*;

      using iterator = T*;
      using const_iterator = const T*;

      constexpr Vector_N() = default;

      constexpr Vector_N(std::initializer_list<T> values) {
        for (auto& value : values) push_back(value);
      }

      constexpr T& operator[](std::size_t index) {
        return m_data[index];
      }

      constexpr const T& operator[](std::size_t index) const {
        return m_data[index];
      }

      constexpr void Clear() {
        m_size = 0;
      }

      constexpr void PushBack(T const& value) {
        m_data[m_size++] = value;
      }

      constexpr void PushBack(T&& value) {
        m_data[m_size++] = std::move(value);
      }

      constexpr void PopBack() {
        m_size--;
      }

      constexpr void Erase(const_iterator it) {
        auto copy_it = (iterator)it;

        while (copy_it != end()) {
          *copy_it = std::move(*(copy_it + 1));
          copy_it++;
        }

        m_size--;
      }

      constexpr iterator Insert(const_iterator it, T const& value) {
        iterator copy_it = end();

        while (copy_it != it) {
          *copy_it = std::move(*(copy_it - 1));
          copy_it--;
        }

        *(iterator)it = value;

        m_size++;

        return (iterator)it;
      }

      constexpr iterator Insert(const_iterator it, T&& value) {
        iterator copy_it = end();

        while (copy_it != it) {
          *copy_it = std::move(*(copy_it - 1));
          copy_it--;
        }

        *(iterator)it = std::move(value);

        m_size++;

        return (iterator)it;
      }

      constexpr reference Front() {
        return m_data[0];
      }

      constexpr const_reference Front() const {
        return m_data[0];
      }

      constexpr reference Back() {
        return m_data[m_size - 1];
      }

      constexpr const_reference Back() const {
        return m_data[m_size - 1];
      }

      [[nodiscard]] constexpr bool Empty() const {
        return m_size == 0;
      }

      [[nodiscard]] constexpr size_t Size() const {
        return m_size;
      }

      constexpr pointer Data() {
        return m_data;
      }

      constexpr const_pointer Data() const {
        return m_data;
      }

      constexpr iterator begin() {
        return (iterator)&m_data[0];
      }

      constexpr iterator end() {
        return (iterator)&m_data[m_size];
      }

      constexpr const_iterator cbegin() const {
        return (const_iterator)&m_data[0];
      }

      constexpr const_iterator cend() const {
        return (const_iterator)&m_data[m_size];
      }

    private:

      T m_data[capacity];

      size_t m_size = 0;
  };

} // namespace zephyr


#pragma once

#include <zephyr/scene/component.hpp>
#include <zephyr/scene/transform.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>
#include <zephyr/panic.hpp>
#include <algorithm>
#include <memory>
#include <span>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace zephyr {

  class SceneNode : public NonCopyable, public NonMoveable {
    public:
      SceneNode() = default;

      explicit SceneNode(std::string name) : m_name{std::move(name)} {}

      [[nodiscard]] SceneNode* GetParent() const {
        return m_parent;
      }

      [[nodiscard]] std::span<const std::unique_ptr<SceneNode>> GetChildren() const {
        return m_children;
      }

      SceneNode* Add(std::unique_ptr<SceneNode> node) {
        node->RemoveFromParent();
        node->m_parent = this;
        m_children.push_back(std::move(node));
        return m_children.back().get();
      }

      template<typename... Args>
      SceneNode* CreateChild(Args&&... args) {
        return Add(std::make_unique<SceneNode>(args...));
      }

      std::unique_ptr<SceneNode> Remove(SceneNode* node) {
        const auto it = std::find_if(m_children.begin(), m_children.end(), [&](const std::unique_ptr<SceneNode>& current_node) {
          return current_node.get() == node;
        });

        if(it == m_children.end()) {
          ZEPHYR_PANIC("The node to be removed is not a child of this node");
        }

        std::unique_ptr<SceneNode> node_ptr = std::move(*it);
        node_ptr->m_parent = nullptr;
        m_children.erase(it);
        return std::move(node_ptr);
      }

      void RemoveFromParent() {
        if(m_parent) {
          m_parent->Remove(this);
        }
      }

      [[nodiscard]] const std::string& GetName() const {
        return m_name;
      }

      void SetName(std::string name) {
        m_name = std::move(name);
      }

      [[nodiscard]] bool IsVisible() const {
        return m_visible;
      }

      void SetVisible(bool visible) {
        m_visible = visible;
      }

      [[nodiscard]] const Transform3D& GetTransform() const {
        return m_transform;
      }

      [[nodiscard]] Transform3D& GetTransform() {
        return m_transform;
      }

      template<typename Functor>
      void Traverse(const Functor& functor) {
        if(!functor(this)) {
          return;
        }

        for(auto& child : GetChildren()) {
          child->Traverse(functor);
        }
      }

      template<typename T>
      bool HasComponent() const {
        return m_components.find(std::type_index{typeid(T)}) != m_components.end();
      }

      template<typename T>
      [[nodiscard]] const T* GetComponent() const {
        return (const T*)const_cast<SceneNode*>(this)->GetComponent<T>();
      }

      template<typename T>
      [[nodiscard]] T* GetComponent() {
        if(!HasComponent<T>()) {
          ZEPHYR_PANIC("Node does not have a component of the type: '{}'", typeid(T).name());
        }

        return m_components[std::type_index{typeid(T)}].get();
      }

      template<typename T, typename... Args>
      T* CreateComponent(Args&&... args) {
        if(HasComponent<T>()) {
          ZEPHYR_PANIC("Node already has a component of the type: '{}'", typeid(T).name());
        }

        std::unique_ptr<T> component = std::make_unique<T>(std::forward(args)...);

        T* component_raw_ptr = component.get();
        m_components[std::type_index{typeid(T)}] = std::move(component);
        return component_raw_ptr;
      }

      template<typename T>
      void RemoveComponent() {
        if(!HasComponent<T>()) {
          ZEPHYR_PANIC("Node does not have a component of the type: '{}'", typeid(T).name());
        }

        m_components.erase(m_components.find(std::type_index{typeid(T)}));
      }

    private:
      SceneNode* m_parent{};
      std::vector<std::unique_ptr<SceneNode>> m_children;
      std::string m_name;
      bool m_visible{true};
      Transform3D m_transform{this};
      std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
  };

} // namespace zephyr

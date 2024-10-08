
#pragma once

#include <zephyr/scene/component.hpp>
#include <zephyr/scene/scene_graph.hpp>
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

class SceneNode : public std::enable_shared_from_this<SceneNode>, NonCopyable, NonMoveable {
    struct Private {};

  public:
    explicit SceneNode(Private) {};
    SceneNode(Private, std::string name, SceneGraph* scene_graph = nullptr) : m_name{std::move(name)}, m_scene_graph{scene_graph} {}

   ~SceneNode() {
      // TODO(fleroviux): debate if we should still signal to the scene graph that the children nodes have been unmounted.
      // As far as I can tell at this point the node would only be bound to a scene graph if the entire scene graph is being destructed.
      for(const auto& child : m_children) {
        child->m_parent = nullptr;
      }
    }

    template<typename... Args>
    static std::shared_ptr<SceneNode> New(Args&&... args) {
      return std::make_shared<SceneNode>(Private{}, std::forward<Args>(args)...);
    }

    [[nodiscard]] std::shared_ptr<SceneNode> GetSharedPtr() {
      return shared_from_this();
    }

    [[nodiscard]] SceneNode* GetParent() const {
      return m_parent;
    }

    [[nodiscard]] std::span<const std::shared_ptr<SceneNode>> GetChildren() const {
      return m_children;
    }

    void Add(std::shared_ptr<SceneNode> node) {
      SceneNode* node_raw = node.get();
      node->RemoveFromParent();
      node->m_parent = this;
      m_children.push_back(std::move(node));

      // If this node belongs to a scene graph, register the new child tree to that scene graph.
      if(m_scene_graph) {
        node_raw->Traverse([this](SceneNode* child_node) {
          child_node->m_scene_graph = m_scene_graph;
          return true;
        });
        m_scene_graph->SignalNodeMounted(node_raw);
      }
    }

    template<typename... Args>
    std::shared_ptr<SceneNode> CreateChild(Args&&... args) {
      std::shared_ptr<SceneNode> node = New(std::forward<Args>(args)...);
      Add(node);
      return std::move(node);
    }

    std::shared_ptr<SceneNode> Remove(SceneNode* node) {
      const auto it = std::find_if(m_children.begin(), m_children.end(), [&](const std::shared_ptr<SceneNode>& current_node) {
        return current_node.get() == node;
      });

      if(it == m_children.end()) {
        ZEPHYR_PANIC("The node to be removed is not a child of this node");
      }

      std::shared_ptr<SceneNode> node_ptr = std::move(*it);
      node_ptr->m_parent = nullptr;
      m_children.erase(it);

      // If this node belongs to a scene graph, unregister the removed child tree from that scene graph.
      if(m_scene_graph) {
        node_ptr->Traverse([this](SceneNode* child_node) {
          child_node->m_scene_graph = nullptr;
          return true;
        });
        m_scene_graph->SignalNodeRemoved(node_ptr.get());
      }

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
      return m_is_visible;
    }

    void SetVisible(bool visible) {
      if(m_is_visible == visible) {
        return;
      }
      m_is_visible = visible;
      if(m_scene_graph) {
        m_scene_graph->SignalNodeVisibilityChanged(this, visible);
      }
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
    [[nodiscard]] const T& GetComponent() const {
      return (const T&)const_cast<SceneNode*>(this)->GetComponent<T>();
    }

    template<typename T>
    [[nodiscard]] T& GetComponent() {
      if(!HasComponent<T>()) {
        ZEPHYR_PANIC("Node does not have a component of the type: '{}'", typeid(T).name());
      }
      return (T&)*m_components[std::type_index{typeid(T)}];
    }

    template<typename T, typename... Args>
    T& CreateComponent(Args&&... args) {
      if(HasComponent<T>()) {
        ZEPHYR_PANIC("Node already has a component of the type: '{}'", typeid(T).name());
      }

      std::unique_ptr<T> component = std::make_unique<T>(std::forward<Args>(args)...);

      T* component_raw_ptr = component.get();
      m_components[std::type_index{typeid(T)}] = std::move(component);
      if(m_scene_graph) {
        m_scene_graph->SignalComponentMounted(this, std::type_index{typeid(T)});
      }
      return *component_raw_ptr;
    }

    template<typename T>
    void RemoveComponent() {
      if(!HasComponent<T>()) {
        ZEPHYR_PANIC("Node does not have a component of the type: '{}'", typeid(T).name());
      }
      m_components.erase(m_components.find(std::type_index{typeid(T)}));
      if(m_scene_graph) {
        m_scene_graph->SignalComponentRemoved(this, std::type_index{typeid(T)});
      }
    }

    [[nodiscard]] const std::unordered_map<std::type_index, std::unique_ptr<Component>>& GetComponents() const {
      return m_components;
    }

  private:
    friend Transform3D;

    SceneGraph* m_scene_graph{};
    SceneNode* m_parent{};
    std::vector<std::shared_ptr<SceneNode>> m_children{};
    std::string m_name{};
    bool m_is_visible{true};
    Transform3D m_transform{this};
    std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components{};
};

} // namespace zephyr

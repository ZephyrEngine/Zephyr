
#pragma once

#include <zephyr/math/matrix4.hpp>
#include <zephyr/math/rotation.hpp>
#include <zephyr/math/vector.hpp>
#include <zephyr/non_copyable.hpp>
#include <zephyr/non_moveable.hpp>
#include <zephyr/panic.hpp>
#include <algorithm>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace zephyr {

  class Transform3D {
    public:
      [[nodiscard]] const Vector3& GetPosition() const {
        return m_position;
      }

      [[nodiscard]] Vector3& GetPosition() {
        return m_position;
      }

      [[nodiscard]] const Vector3& GetScale() const {
        return m_scale;
      }

      [[nodiscard]] Vector3& GetScale() {
        return m_scale;
      }

      [[nodiscard]] const Rotation& GetRotation() const {
        return m_rotation;
      }

      [[nodiscard]] Rotation& GetRotation() {
        return m_rotation;
      }

      [[nodiscard]] const Matrix4& GetLocal() const {
        return m_local_matrix;
      }

      [[nodiscard]] const Matrix4& GetWorld() const {
        return m_world_matrix;
      }

      void UpdateLocal() {
        m_local_matrix = m_rotation.GetAsMatrix4();
        m_local_matrix.X() *= m_scale.X();
        m_local_matrix.Y() *= m_scale.Y();
        m_local_matrix.Z() *= m_scale.Z();
        m_local_matrix.W() = Vector4{m_position, 0.0f};
      }

      // @todo: this is not really a good solution
      void UpdateWorld(const Transform3D& parent) {
        m_world_matrix = parent.GetWorld() * m_local_matrix;
      }

    private:
      Vector3 m_position;
      Vector3 m_scale;
      Rotation m_rotation;
      Matrix4 m_local_matrix;
      Matrix4 m_world_matrix;
  };

  class SceneNode : public NonCopyable, NonMoveable {
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

    private:
      SceneNode* m_parent{};
      std::vector<std::unique_ptr<SceneNode>> m_children;
      std::string m_name;
      bool m_visible{true};
      Transform3D m_transform;
  };

} // namespace zephyr

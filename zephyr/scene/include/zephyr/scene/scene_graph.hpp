
#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <typeindex>

namespace zephyr {

  class SceneNode;

  class SceneGraph {
    public:
      SceneGraph();

      [[nodiscard]] const SceneNode* GetRoot() const {
        return m_root_node.get();
      }

      [[nodiscard]] SceneNode* GetRoot() {
        return m_root_node.get();
      }

      void UpdateTransforms();
      bool QueryNodeWorldVisibility(const SceneNode* node);

    private:
      friend SceneNode;
      friend class Transform3D;

      void SignalNodeMounted(SceneNode* node);
      void SignalNodeRemoved(SceneNode* node);
      void SignalComponentMounted(SceneNode* node, std::type_index type_index);
      void SignalComponentRemoved(SceneNode* node, std::type_index type_index);
      void SignalNodeTransformChanged(SceneNode* node);
      void SignalNodeVisibilityChanged(SceneNode* node, bool visible);

      std::shared_ptr<SceneNode> m_root_node{};
      std::vector<SceneNode*> m_pending_nodes_with_dirty_transforms{};
      std::vector<SceneNode*> m_nodes_with_dirty_transform{};
      std::unordered_map<const SceneNode*, bool> m_node_world_visibility{}; //< Tracks the world visibility of nodes
  };

} // namespace zephyr

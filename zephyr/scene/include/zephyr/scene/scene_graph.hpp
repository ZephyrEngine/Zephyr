
#pragma once

#include <memory>
#include <vector>

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

    private:
      friend SceneNode;
      friend class Transform3D;

      void SignalNodeMounted(SceneNode* node);
      void SignalNodeRemoved(SceneNode* node);
      void SignalNodeTransformChanged(SceneNode* node);

      std::shared_ptr<SceneNode> m_root_node{};
      std::vector<SceneNode*> m_nodes_with_dirty_transform{};
  };

} // namespace zephyr

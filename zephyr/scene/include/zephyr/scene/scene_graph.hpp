
#pragma once

#include <zephyr/integer.hpp>
#include <memory>
#include <span>
#include <vector>
#include <unordered_map>
#include <typeindex>

namespace zephyr {

  class SceneNode;

  struct ScenePatch {
    enum class Type : u8 {
      NodeMounted,
      NodeRemoved,
      ComponentMounted,
      ComponentRemoved,
      NodeTransformChanged
    };

    Type type;
    std::shared_ptr<SceneNode> node;
    std::type_index component_type{typeid(void)};
  };

  class SceneGraph {
    public:
      SceneGraph();

      [[nodiscard]] const SceneNode* GetRoot() const {
        return m_root_node.get();
      }

      [[nodiscard]] SceneNode* GetRoot() {
        return m_root_node.get();
      }

      void ClearScenePatches() {
        m_scene_patches.clear();
      }

      [[nodiscard]] std::span<ScenePatch const> GetScenePatches() {
        return m_scene_patches;
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
      std::vector<ScenePatch> m_scene_patches{};
  };

} // namespace zephyr


#include <zephyr/scene/scene_graph.hpp>
#include <zephyr/scene/scene_node.hpp>
#include <algorithm>

namespace zephyr {

  SceneGraph::SceneGraph() {
    m_root_node = SceneNode::New("SceneRoot", this);
    SignalNodeMounted(m_root_node.get());
  }

  void SceneGraph::UpdateTransforms() {
    for(const auto node : m_nodes_with_dirty_transform) {
      node->GetTransform().UpdateLocal();
      node->GetTransform().UpdateWorld();
    }
    m_nodes_with_dirty_transform.clear();
  }

  void SceneGraph::SignalNodeMounted(SceneNode* node) {
    SignalNodeTransformChanged(node);
  }

  void SceneGraph::SignalNodeRemoved(SceneNode* node) {
    node->Traverse([this](SceneNode* child_node) {
      const auto match = std::ranges::find(m_nodes_with_dirty_transform, child_node);
      if(match != m_nodes_with_dirty_transform.end()) {
        m_nodes_with_dirty_transform.erase(match);
      }
      return true;
    });
  }

  void SceneGraph::SignalNodeTransformChanged(SceneNode* node) {
    // TODO(fleroviux): try to efficiently eliminate redundant updates.
    node->Traverse([this](SceneNode* child_node) {
      m_nodes_with_dirty_transform.push_back(child_node);
      return true;
    });
  }

} // namespace zephyr

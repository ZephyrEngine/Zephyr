
#include <zephyr/scene/scene_graph.hpp>
#include <zephyr/scene/scene_node.hpp>

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
  }

  void SceneGraph::SignalNodeTransformChanged(SceneNode* node) {
    // TODO(fleroviux): avoid updating nodes more than once.
    // If a node is marked for update once and then later again, ideally skip the first update.
    node->Traverse([this](SceneNode* child_node) {
      m_nodes_with_dirty_transform.push_back(child_node);
      return true;
    });
  }

} // namespace zephyr

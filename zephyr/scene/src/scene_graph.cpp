
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

    if(QueryNodeWorldVisibility(node)) { // TODO(fleroviux): this check is slow!
      m_scene_patches.push_back({
        .type = ScenePatch::Type::NodeTransformChanged,
        .node = node->GetSharedPtr()
      });
    }
  }
  m_nodes_with_dirty_transform.clear();
}

bool SceneGraph::QueryNodeWorldVisibility(const SceneNode* node) {
  return m_node_world_visibility[node];
}

void SceneGraph::SignalNodeMounted(SceneNode* node) {
  SignalNodeTransformChanged(node);
  SignalNodeVisibilityChanged(node, node->IsVisible());

  if(QueryNodeWorldVisibility(node)) {
    node->Traverse([this](SceneNode* child_node) {
      if(child_node->IsVisible()) {
        m_scene_patches.push_back({
          .type = ScenePatch::Type::NodeMounted,
          .node = child_node->GetSharedPtr()
        });
        return true;
      }

      return false;
    });
  }
}

void SceneGraph::SignalNodeRemoved(SceneNode* node) {
  if(QueryNodeWorldVisibility(node)) {
    node->Traverse([this](SceneNode* child_node) {
      if(child_node->IsVisible()) {
        m_scene_patches.push_back({
          .type = ScenePatch::Type::NodeRemoved,
          .node = child_node->GetSharedPtr()
        });
        return true;
      }

      return false;
    });
  }

  node->Traverse([this](SceneNode* child_node) {
    const auto match = std::ranges::find(m_nodes_with_dirty_transform, child_node);
    if(match != m_nodes_with_dirty_transform.end()) {
      m_nodes_with_dirty_transform.erase(match);
    }
    return true;
  });
}

void SceneGraph::SignalComponentMounted(SceneNode* node, std::type_index type_index) {
  if(QueryNodeWorldVisibility(node)) {
    m_scene_patches.push_back({
      .type = ScenePatch::Type::ComponentMounted,
      .node = node->GetSharedPtr(),
      .component_type = type_index
    });
  }
}

void SceneGraph::SignalComponentRemoved(SceneNode* node, std::type_index type_index) {
  if(QueryNodeWorldVisibility(node)) {
    m_scene_patches.push_back({
      .type = ScenePatch::Type::ComponentRemoved,
      .node = node->GetSharedPtr(),
      .component_type = type_index
    });
  }
}

void SceneGraph::SignalNodeTransformChanged(SceneNode* node) {
  // TODO(fleroviux): try to efficiently eliminate redundant updates.
  node->Traverse([this](SceneNode* child_node) {
    m_nodes_with_dirty_transform.push_back(child_node);
    return true;
  });
}

void SceneGraph::SignalNodeVisibilityChanged(SceneNode* node, bool visible) {
  if(visible) {
    SceneNode* parent_node = node->GetParent();

    if(!parent_node || m_node_world_visibility[parent_node]) {
      m_node_world_visibility[node] = true;

      node->Traverse([this](SceneNode* child_node) {
        if(child_node->IsVisible()) {
          m_node_world_visibility[child_node] = true;

          m_scene_patches.push_back({
            .type = ScenePatch::Type::NodeMounted,
            .node = child_node->GetSharedPtr()
          });
          return true;
        }

        return false;
      });
    }
  } else if(m_node_world_visibility[node]) {
    node->Traverse([this](SceneNode* child_node) {
      if(m_node_world_visibility[child_node]) {
        m_node_world_visibility[child_node] = false;

        m_scene_patches.push_back({
          .type = ScenePatch::Type::NodeRemoved,
          .node = child_node->GetSharedPtr()
        });
        return true;
      }

      return false;
    });
  }
}

} // namespace zephyr

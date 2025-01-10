
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <eastl/hash_map.h>
#include <eastl/hash_set.h>
#include <memory>
#include <vector>

// TODO: implement clean texture class
#include <zephyr/renderer/resource/resource.hpp>
namespace zephyr {
class Texture final : public Resource {
  public:
    Texture(u32 width, u32 height) {
      m_data = new u32[width * height];
      m_width = width;
      m_height = height;
    }
   ~Texture() override {
      delete[] m_data;
    }
    u32* m_data{};
    u32 m_width{};
    u32 m_height{};
};
} // namespace zephyr

// TODO: implement clean RenderTexture class
namespace zephyr {
class RenderTexture {}; // placeholder!
} // namespace zephyr

namespace zephyr {

class TextureCache {
  public:
    explicit TextureCache(std::shared_ptr<RenderBackend> render_backend) : m_render_backend{std::move(render_backend)} {}

   ~TextureCache();

    // Game Thread API:
    void QueueTasksForRenderThread();
    void IncrementTextureRefCount(const Texture* texture);
    void DecrementTextureRefCount(const Texture* texture);

    // Render Thread API:
    void ProcessQueuedTasks();
    // RenderTexture* GetCachedRenderTexture(const Texture* texture) const;

  private:
    struct TextureState {
      bool uploaded{false};
      u64 current_version{};
      size_t ref_count{};
      VoidEvent::SubID destruct_event_subscription;
    };

    struct UploadTask {
      const Texture* texture;
      u8* raw_data;
      u32 width;
      u32 height;
    };

    struct DeleteTask {
      const Texture* texture;
    };

    // Game Thread:
    void QueueUploadTasksForUsedTextures();
    void QueueDeleteTasksFromPreviousFrame();
    void QueueTextureUploadTaskIfNeeded(const Texture* texture);
    void QueueTextureDeleteTaskForNextFrame(const Texture* texture);

    // Render Thread:
    void ProcessQueuedDeleteTasks();
    void ProcessQueuedUploadTasks();

    std::shared_ptr<RenderBackend> m_render_backend;
    eastl::hash_set<const Texture*> m_used_texture_set{};
    eastl::hash_map<const Texture*, TextureState> m_texture_state_table{};
    mutable eastl::hash_map<const Texture*, RenderTexture*> m_render_texture_table{};
    std::vector<UploadTask> m_upload_tasks{};
    std::vector<DeleteTask> m_delete_tasks[2]{};
};

} // namespace zephyr

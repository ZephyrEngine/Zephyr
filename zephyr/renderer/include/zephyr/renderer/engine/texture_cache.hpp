
#pragma once

#include <zephyr/renderer/backend/render_backend.hpp>
#include <zephyr/renderer/resource/texture.hpp>
#include <zephyr/integer.hpp>
#include <eastl/hash_map.h>
#include <eastl/hash_set.h>
#include <memory>
#include <span>
#include <vector>

namespace zephyr {

class TextureCache {
  public:
    explicit TextureCache(std::shared_ptr<RenderBackend> render_backend) : m_render_backend{std::move(render_backend)} {}

   ~TextureCache();

    // Game Thread API:
    void QueueTasksForRenderThread();
    void IncrementTextureRefCount(const TextureBase* texture);
    void DecrementTextureRefCount(const TextureBase* texture);

    // Render Thread API:
    void ProcessQueuedTasks();
    // RenderTexture* GetCachedRenderTexture(const TextureBase* texture) const;

  private:
    struct TextureState {
      bool uploaded{false};
      u64 current_version{};
      size_t ref_count{};
      VoidEvent::SubID destruct_event_subscription;
    };

    struct UploadTask {
      const TextureBase* texture;
      std::span<const u8> raw_data;
      u32 width;
      u32 height;
    };

    struct DeleteTask {
      const TextureBase* texture;
    };

    // Game Thread:
    void QueueUploadTasksForUsedTextures();
    void QueueDeleteTasksFromPreviousFrame();
    void QueueTextureUploadTaskIfNeeded(const TextureBase* texture);
    void QueueTextureDeleteTaskForNextFrame(const TextureBase* texture);

    // Render Thread:
    void ProcessQueuedDeleteTasks();
    void ProcessQueuedUploadTasks();

    std::shared_ptr<RenderBackend> m_render_backend;
    eastl::hash_set<const TextureBase*> m_used_texture_set{};
    eastl::hash_map<const TextureBase*, TextureState> m_texture_state_table{};
    mutable eastl::hash_map<const TextureBase*, RenderTexture*> m_render_texture_table{};
    std::vector<UploadTask> m_upload_tasks{};
    std::vector<DeleteTask> m_delete_tasks[2]{};
};

} // namespace zephyr

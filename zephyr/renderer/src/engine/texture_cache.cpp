
#include <zephyr/renderer/engine/texture_cache.hpp>
#include <algorithm>

namespace zephyr {

TextureCache::~TextureCache() {
  // Ensure that we do not receive any destruction callbacks from textures that outlive this texture cache.
  for(const auto& [texture, state] : m_texture_state_table) {
    if(state.uploaded) {
      texture->OnBeforeDestruct().Unsubscribe(state.destruct_event_subscription);
    }
  }
}

void TextureCache::QueueTasksForRenderThread() {
  // Queue (re-)uploads for all textures used in the submitted frame which are either new or have changed since the last frame.
  QueueUploadTasksForUsedTextures();

  // Queue eviction of textures which had been deleted in the previously submitted frame.
  QueueDeleteTasksFromPreviousFrame();
}

void TextureCache::IncrementTextureRefCount(const Texture* texture) {
  if(++m_texture_state_table[texture].ref_count == 1u) {
    m_used_texture_set.insert(texture);
  }
}

void TextureCache::DecrementTextureRefCount(const Texture* texture) {
  if(--m_texture_state_table[texture].ref_count == 0u) {
    m_used_texture_set.erase(texture);
  }
}

void TextureCache::QueueUploadTasksForUsedTextures() {
  for(const Texture* texture : m_used_texture_set) {
    QueueTextureUploadTaskIfNeeded(texture);
  }
}

void TextureCache::QueueDeleteTasksFromPreviousFrame() {
  std::swap(m_delete_tasks[0], m_delete_tasks[1]);
}

void TextureCache::QueueTextureUploadTaskIfNeeded(const Texture* texture) {
  TextureState& state = m_texture_state_table[texture];

  if(!state.uploaded || state.current_version != texture->CurrentVersion()) {
    // TODO(fleroviux): allocate staging memory from a dedicated allocator?
    const u32 width = texture->m_width;
    const u32 height = texture->m_height;
    u8* staging_data = new u8[width * height * sizeof(u32)];
    std::memcpy(staging_data, texture->m_data, width * height * sizeof(u32));

    m_upload_tasks.push_back({
      .texture = texture,
      .raw_data = staging_data,
      .width = width,
      .height = height
    });

    if(!state.uploaded) {
      state.destruct_event_subscription = texture->OnBeforeDestruct().Subscribe(
        std::bind(&TextureCache::QueueTextureDeleteTaskForNextFrame, this, texture));
    }

    state.uploaded = true;
    state.current_version = texture->CurrentVersion();
  }
}

void TextureCache::QueueTextureDeleteTaskForNextFrame(const Texture* texture) {
  /**
   * Queue the texture for eviction from the cache, when the next frame begins rendering.
   * This ensures that the texture is only evicted after the current frame has fully rendered.
   */
  m_delete_tasks[1].push_back({.texture = texture});
  m_texture_state_table.erase(texture);
}

void TextureCache::ProcessQueuedTasks() {
  ProcessQueuedDeleteTasks();
  ProcessQueuedUploadTasks();
}

void TextureCache::ProcessQueuedDeleteTasks() {
  for(const auto& delete_task : m_delete_tasks[0]) {
    // TODO(fleroviux)
  }

  m_delete_tasks[0].clear();
}

void TextureCache::ProcessQueuedUploadTasks() {
  for(const auto& upload_task : m_upload_tasks) {
    // TODO(fleroviux)
    delete[] upload_task.raw_data;
  }

  m_upload_tasks.clear();
}


} // namespace zephyr

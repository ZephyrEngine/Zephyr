
#include <zephyr/renderer/engine/texture_cache.hpp>
#include <zephyr/renderer/resource/texture_2d.hpp>
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

void TextureCache::IncrementTextureRefCount(const TextureBase* texture) {
  if(++m_texture_state_table[texture].ref_count == 1u) {
    m_used_texture_set.insert(texture);
  }
}

void TextureCache::DecrementTextureRefCount(const TextureBase* texture) {
  if(--m_texture_state_table[texture].ref_count == 0u) {
    m_used_texture_set.erase(texture);
  }
}

void TextureCache::QueueUploadTasksForUsedTextures() {
  for(const TextureBase* texture : m_used_texture_set) {
    QueueTextureUploadTaskIfNeeded(texture);
  }
}

void TextureCache::QueueDeleteTasksFromPreviousFrame() {
  std::swap(m_delete_tasks[0], m_delete_tasks[1]);
}

void TextureCache::QueueTextureUploadTaskIfNeeded(const TextureBase* texture) {
  TextureState& state = m_texture_state_table[texture];

  if(!state.uploaded || state.current_version != texture->CurrentVersion()) {
    // TODO(fleroviux): implement logic for uploads of different texture types (e.g. 3D texture or cube map)
    // TODO(fleroviux): allocate staging memory from a dedicated allocator?
    const u32 width = dynamic_cast<const Texture2D*>(texture)->GetWidth();
    const u32 height = dynamic_cast<const Texture2D*>(texture)->GetHeight();
    u8* staging_data = new u8[width * height * sizeof(u32)];
    std::memcpy(staging_data, texture->Data(), width * height * sizeof(u32));

    m_upload_tasks.push_back({
      .texture = texture,
      .raw_data = {staging_data, width * height * sizeof(u32)},
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

void TextureCache::QueueTextureDeleteTaskForNextFrame(const TextureBase* texture) {
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
    RenderTexture* render_texture = m_render_texture_table[delete_task.texture];
    if(render_texture) {
      m_render_backend->DestroyRenderTexture(render_texture);
    }
    m_render_texture_table.erase(delete_task.texture);
  }

  m_delete_tasks[0].clear();
}

void TextureCache::ProcessQueuedUploadTasks() {
  for(const auto& upload_task : m_upload_tasks) {
    const TextureBase* texture = upload_task.texture;
    RenderTexture* render_texture = m_render_texture_table[texture];

    if(!render_texture) {
      render_texture = m_render_backend->CreateRenderTexture(upload_task.width, upload_task.height);
      m_render_texture_table[texture] = render_texture;
    }
    m_render_backend->UpdateRenderTextureData(render_texture, upload_task.raw_data);

    delete[] upload_task.raw_data.data();
  }

  m_upload_tasks.clear();
}

} // namespace zephyr

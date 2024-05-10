
#include <zephyr/renderer/backend/render_backend_ogl.hpp>
#include <zephyr/renderer/backend/render_backend_vk.hpp>
#include <zephyr/renderer/component/camera.hpp>

#include "gltf_loader.hpp"
#include "main_window.hpp"

static const bool enable_validation_layers = true;
static const bool benchmark_scene_size = true;

namespace zephyr {

  MainWindow::~MainWindow() {
    // HACK: make sure that render engine and render backend are cleanly destroyed first, before we destroy the VkInstance etc.
    m_render_engine = nullptr;

    #ifdef ZEPHYR_OPENGL
      CleanupOpenGL();
    #else
      CleanupVulkan();
    #endif
  }

  void MainWindow::Run() {
    Setup();
    MainLoop();
  }

  void MainWindow::Setup() {
    if(benchmark_scene_size) {
      CreateBenchmarkScene();
    } else {
      CreateScene();
    }

    #ifdef ZEPHYR_OPENGL
      CreateOpenGLEngine();
    #else
      CreateVulkanEngine();
    #endif
  }

  void MainWindow::MainLoop() {
    SDL_Event event{};

    f32 euler_x = 0.0f;
    f32 euler_y = 0.0f;

    while(true) {
      while(SDL_PollEvent(&event)) {
        if(event.type == SDL_QUIT) {
          return;
        }

        if(event.type == SDL_KEYUP) {
          SDL_KeyboardEvent* key_event = (SDL_KeyboardEvent*)&event;
          switch(key_event->keysym.sym) {
            case SDLK_z: {
              if(m_behemoth_scene) {
                m_scene_root->Remove(m_behemoth_scene.get());
                m_behemoth_scene.reset();
              }
              break;
            }
            case SDLK_x: {
              if(m_behemoth_scene) {
                m_behemoth_scene->SetVisible(!m_behemoth_scene->IsVisible());
              }
              break;
            }
          }
        }
      }

      const u8* key_state = SDL_GetKeyboardState(nullptr);

      Vector3&  camera_position = m_camera_node->GetTransform().GetPosition();
      const f32 delta_p = 0.075f;
      const f32 delta_r = 0.01f;
      if(key_state[SDL_SCANCODE_W]) camera_position -= m_camera_node->GetTransform().GetLocal().Z().XYZ() * delta_p;
      if(key_state[SDL_SCANCODE_S]) camera_position += m_camera_node->GetTransform().GetLocal().Z().XYZ() * delta_p;
      if(key_state[SDL_SCANCODE_A]) camera_position -= m_camera_node->GetTransform().GetLocal().X().XYZ() * delta_p;
      if(key_state[SDL_SCANCODE_D]) camera_position += m_camera_node->GetTransform().GetLocal().X().XYZ() * delta_p;
      if(key_state[SDL_SCANCODE_LEFT])  euler_y += delta_r;
      if(key_state[SDL_SCANCODE_RIGHT]) euler_y -= delta_r;
      if(key_state[SDL_SCANCODE_UP])    euler_x += delta_r;
      if(key_state[SDL_SCANCODE_DOWN])  euler_x -= delta_r;
      m_camera_node->GetTransform().GetRotation().SetFromEuler(euler_x, euler_y, 0.0f);

      RenderFrame();
    }
  }

  void MainWindow::RenderFrame() {
    m_scene_root->Traverse([&](SceneNode* node) {
      node->GetTransform().UpdateLocal();
      node->GetTransform().UpdateWorld();
      return true;
    });

    m_render_engine->RenderScene(m_scene_root.get());

    m_frame++;

    UpdateFramesPerSecondCounter();
  }

  void MainWindow::UpdateFramesPerSecondCounter() {
    const auto time_point_now = std::chrono::steady_clock::now();

    const auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      time_point_now - m_time_point_last_update).count();

    m_fps_counter++;

    if(time_elapsed >= 1000) {
      const f32 fps = (f32)m_fps_counter * 1000.0f / (f32)time_elapsed;
      fmt::print("{} fps\n", fps);
      m_fps_counter = 0;
      m_time_point_last_update = std::chrono::steady_clock::now();
    }
  }

  void MainWindow::CreateVulkanEngine() {
    const VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Vulkan Playground",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Vulkan Playground Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_MAKE_VERSION(1, 0, 0)
    };

    m_window = SDL_CreateWindow(
      "Zephyr Next (Vulkan)",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      1920,
      1080,
      SDL_WINDOW_VULKAN
    );

    std::vector<const char*> required_extension_names{};
    uint extension_count;
    SDL_Vulkan_GetInstanceExtensions(m_window, &extension_count, nullptr);
    required_extension_names.resize(extension_count);
    SDL_Vulkan_GetInstanceExtensions(m_window, &extension_count, required_extension_names.data());

    std::vector<const char*> required_layer_names{};
    if(enable_validation_layers && VulkanInstance::QueryInstanceLayerSupport("VK_LAYER_KHRONOS_validation")) {
      required_layer_names.push_back("VK_LAYER_KHRONOS_validation");
    }

    m_vk_instance = VulkanInstance::Create(app_info, required_extension_names, required_layer_names);

    if(!SDL_Vulkan_CreateSurface(m_window, m_vk_instance->Handle(), &m_vk_surface)) {
      ZEPHYR_PANIC("Failed to create a Vulkan surface for the window");
    }

    m_render_engine = std::make_unique<RenderEngine>(CreateVulkanRenderBackend({
      .vk_instance = m_vk_instance,
      .vk_surface = m_vk_surface
    }));
  }

  void MainWindow::CreateScene() {
    m_scene_root = SceneNode::New();

    m_camera_node = m_scene_root->CreateChild("RenderCamera");
    m_camera_node->CreateComponent<PerspectiveCameraComponent>(45.0f, 16.f / 9.f, 0.01f, 100.f);
    m_camera_node->GetTransform().GetPosition() = {0.f, 0.f, 5.f};

    GLTFLoader gltf_loader{};
    std::shared_ptr<SceneNode> gltf_scene_1 = gltf_loader.Parse("models/DamagedHelmet/DamagedHelmet.gltf");
    gltf_scene_1->GetTransform().GetPosition() = {1.0f, 0.0f, -5.0f};
    gltf_scene_1->GetTransform().GetRotation().SetFromEuler(1.5f, 0.0f, 0.0f);
    m_scene_root->Add(std::move(gltf_scene_1));

    m_scene_root->Add(gltf_loader.Parse("models/triangleWithoutIndices/TriangleWithoutIndices.gltf"));
    //m_scene_root->Add(gltf_loader.Parse("models/triangle/Triangle.gltf"));

    m_behemoth_scene = gltf_loader.Parse("models/Behemoth/scene.gltf");
    m_behemoth_scene->GetTransform().GetPosition() = Vector3{-1.0f, 0.0f, -5.0f};
    m_behemoth_scene->GetTransform().GetRotation().SetFromEuler(-M_PI * 0.5, M_PI, 0.0f);
    m_behemoth_scene->GetTransform().GetScale() = {0.5f, 0.5f, 0.5f};
    m_scene_root->Add(m_behemoth_scene);
  }

  void MainWindow::CreateBenchmarkScene() {
    m_scene_root = SceneNode::New();

    // TODO(fleroviux): engine crashes when there is no camera in the scene! VERY BAD!!!
    m_camera_node = m_scene_root->CreateChild("RenderCamera");
    m_camera_node->CreateComponent<PerspectiveCameraComponent>(45.0f, 16.f / 9.f, 0.01f, 100.f);
    m_camera_node->GetTransform().GetPosition() = {0.f, 0.f, 5.f};

    // TODO(fleroviux): fix cube geometry is rendered incorrectly.

    /**
     *   4-------5
     *  /|      /|
     * 0-------1 |
     * | 6-----|-7
     * |/      |/
     * 2-------3
     */
    RenderGeometryLayout layout{};
    layout.AddAttribute(RenderGeometryAttribute::Position);
    layout.AddAttribute(RenderGeometryAttribute::Color);

    std::shared_ptr<Geometry> cube_geometry = std::make_shared<Geometry>(layout, 8, 36);

    auto positions = cube_geometry->GetPositions();
    positions[0] = Vector3{-1.0, -1.0,  1.0};
    positions[1] = Vector3{ 1.0, -1.0,  1.0};
    positions[2] = Vector3{-1.0,  1.0,  1.0};
    positions[3] = Vector3{ 1.0,  1.0,  1.0};
    positions[4] = Vector3{-1.0, -1.0, -1.0};
    positions[5] = Vector3{ 1.0, -1.0, -1.0};
    positions[6] = Vector3{-1.0,  1.0, -1.0};
    positions[7] = Vector3{ 1.0,  1.0, -1.0};

    auto colors = cube_geometry->GetColors();
    if(colors.IsValid()) {
      colors[0] = Vector4{1.0, 0.0, 0.0, 1.0};
      colors[1] = Vector4{0.0, 1.0, 0.0, 1.0};
      colors[2] = Vector4{0.0, 0.0, 1.0, 1.0};
      colors[3] = Vector4{1.0, 0.0, 1.0, 1.0};
      colors[4] = Vector4{1.0, 1.0, 0.0, 1.0};
      colors[5] = Vector4{0.0, 1.0, 1.0, 1.0};
      colors[6] = Vector4{1.0, 1.0, 1.0, 1.0};
      colors[7] = Vector4{0.0, 0.0, 0.0, 1.0};
    }

    auto indices = cube_geometry->GetIndices();
    u32 index_data[] {
      // front
      0, 1, 2,
      1, 3, 2,

      // back
      4, 5, 6,
      5, 7, 6,

      // left
      0, 4, 6,
      0, 6, 2,

      // right
      1, 5, 7,
      1, 7, 3,

      // top
      4, 1, 0,
      4, 5, 1,

      // bottom
      6, 3, 2,
      6, 7, 3
    };
    std::copy_n(index_data, sizeof(index_data) / sizeof(u32), indices.begin());

    const int grid_size = 37;

    for(int x = -grid_size / 2; x < grid_size / 2; x++) {
      for(int y = -grid_size / 2; y < grid_size / 2; y++) {
        for(int z = -grid_size / 2; z < grid_size / 2; z++) {
          std::shared_ptr<SceneNode> cube = m_scene_root->CreateChild("Cube");
          cube->CreateComponent<MeshComponent>(cube_geometry);
          cube->GetTransform().GetPosition() = {(f32)x, (f32)y, (f32)-z};
          cube->GetTransform().GetScale() = {0.1, 0.1, 0.1};
        }
      }
    }
  }

  void MainWindow::CleanupVulkan() {
    vkDestroySurfaceKHR(m_vk_instance->Handle(), m_vk_surface, nullptr);
    SDL_DestroyWindow(m_window);
  }

  void MainWindow::CreateOpenGLEngine() {
    m_window = SDL_CreateWindow(
      "Zephyr Next (OpenGL)",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      1920,
      1080,
      SDL_WINDOW_OPENGL
    );

    m_render_engine = std::make_unique<RenderEngine>(CreateOpenGLRenderBackendForSDL2(m_window));
  }

  void MainWindow::CleanupOpenGL() {
  }

} // namespace zephyr
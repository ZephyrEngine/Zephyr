
#include <zephyr/renderer/backend/render_backend_ogl.hpp>
#include <zephyr/renderer/backend/render_backend_vk.hpp>
#include <zephyr/renderer/component/mesh.hpp>

#include "main_window.hpp"

static const bool enable_validation_layers = true;

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
    CreateScene();

    #ifdef ZEPHYR_OPENGL
      CreateOpenGLEngine();
    #else
      CreateVulkanEngine();
    #endif
  }

  void MainWindow::MainLoop() {
    SDL_Event event{};

    while(true) {
      while(SDL_PollEvent(&event)) {
        if(event.type == SDL_QUIT) {
          return;
        }
      }

      RenderFrame();
    }
  }

  void MainWindow::RenderFrame() {
    // Update the transform of the second cube
    const float angle = (f32)m_frame * 0.01f;
    auto& cube_b = m_scene_root->GetChildren()[0]->GetChildren()[0];
    cube_b->GetTransform().GetPosition() = Vector3{std::cos(angle), 0.0f, -std::sin(angle)} * 2.0f;
    cube_b->GetTransform().GetRotation().SetFromEuler(0.0f, angle, 0.0f);

    m_scene_root->Traverse([&](SceneNode* node) {
      node->GetTransform().UpdateLocal();
      node->GetTransform().UpdateWorld();
      return true;
    });

    m_render_engine->RenderScene(m_scene_root.get());

    m_frame++;
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
    m_scene_root = std::make_unique<SceneNode>();

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

    SceneNode* cube_a = m_scene_root->CreateChild("Cube A");
    cube_a->CreateComponent<MeshComponent>(cube_geometry);
    cube_a->GetTransform().GetPosition() = Vector3{0.0f, 0.0f, -5.0f};

    SceneNode* cube_b = cube_a->CreateChild("Cube B");
    cube_b->CreateComponent<MeshComponent>(cube_geometry);
    cube_b->GetTransform().GetScale() = Vector3{0.25f, 0.25f, 0.25f};
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
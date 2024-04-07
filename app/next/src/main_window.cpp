
#include <zephyr/renderer2/backend/render_backend_vk.hpp>

#include "main_window.hpp"

static const bool enable_validation_layers = true;

namespace zephyr {

  MainWindow::~MainWindow() {
    Cleanup();
  }

  void MainWindow::Run() {
    Setup();
    MainLoop();
  }

  void MainWindow::Setup() {
    m_window = SDL_CreateWindow(
      "Zephyr Next",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      1920,
      1080,
      SDL_WINDOW_VULKAN
    );

    CreateVkInstance();
    CreateSurface();
    CreateRenderEngine();
    CreateScene();
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

  void MainWindow::CreateVkInstance() {
    const VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Vulkan Playground",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Vulkan Playground Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_MAKE_VERSION(1, 0, 0)
    };

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
  }

  void MainWindow::CreateSurface() {
    if(!SDL_Vulkan_CreateSurface(m_window, m_vk_instance->Handle(), &m_vk_surface)) {
      ZEPHYR_PANIC("Failed to create a Vulkan surface for the window");
    }
  }

  void MainWindow::CreateRenderEngine() {
    m_render_engine = std::make_unique<RenderEngine>(CreateVulkanRenderBackend({
      .vk_instance = m_vk_instance,
      .vk_surface = m_vk_surface
    }));
  }

  void MainWindow::CreateScene() {
    m_scene_root = std::make_unique<SceneNode>();

    //std::shared_ptr<Material> pbr_material = std::make_shared<Material>(std::make_shared<PBRMaterialShader>());

    SceneNode* cube_a = m_scene_root->CreateChild("Cube A");
    //cube_a->CreateComponent<MeshComponent>(m_cube_mesh, pbr_material);
    cube_a->CreateComponent<MeshComponent>();
    cube_a->GetTransform().GetPosition() = Vector3{0.0f, 0.0f, -5.0f};

    SceneNode* cube_b = cube_a->CreateChild("Cube B");
    //cube_b->CreateComponent<MeshComponent>(m_cube_mesh, pbr_material);
    cube_b->CreateComponent<MeshComponent>();
    cube_b->GetTransform().GetScale() = Vector3{0.25f, 0.25f, 0.25f};
  }

  void MainWindow::Cleanup() {
    vkDestroySurfaceKHR(m_vk_instance->Handle(), m_vk_surface, nullptr);
    SDL_DestroyWindow(m_window);
  }

} // namespace zephyr
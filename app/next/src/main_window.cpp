
#include <zephyr/renderer/backend/render_backend_ogl.hpp>
#include <zephyr/renderer/backend/render_backend_vk.hpp>
#include <zephyr/renderer/component/camera.hpp>

#include "gltf_loader.hpp"
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
              m_behemoth_scene->SetVisible(!m_behemoth_scene->IsVisible());
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
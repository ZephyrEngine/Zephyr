
#include <stb_image.h>

#include "main_window.hpp"

#include "renderer/material.hpp"

// Shader compilation test
#include <fstream>
#include <glslang/Include/glslang_c_shader_types.h>
#include <glslang/Public/resource_limits_c.h>
#include <string>

namespace zephyr {

  MainWindow::MainWindow() {
    SetWindowSize(1600, 900);
    SetWindowTitle("Zephyr Runtime");
    Setup();

    {
      GLSLVariableList parameters{};
      parameters.Add<f32>("some_float");
      parameters.Add<bool>("some_bool");
      parameters.Add<Vector2>("some_vec2");
      parameters.Add<int>("some_int");
      parameters.Add<Vector3>("some_vec3");
      parameters.Add<Vector3>("some_vec3_2");
      parameters.Add<Vector4>("some_vec4");
      parameters.Add<uint>("some_uint");
      parameters.Add<uint>("some_uint_2");
      parameters.Add<Matrix4>("some_mat4");
      parameters.Add<Matrix4>("some_mat4_2");
      parameters.Add<int>("int_array_1", 16);
      parameters.Add<Matrix4>("mat4_array_1", 16);

      STD430BufferLayout layout{parameters};

      for(auto& variable : layout.GetVariables()) {
        if(variable.array_size != 0u) {
          ZEPHYR_INFO("[{}, \t{}]\t{} {}[{}]", variable.buffer_offset, variable.data_size, (std::string)variable.type, variable.name, variable.array_size);
        } else {
          ZEPHYR_INFO("[{}, \t{}]\t{} {}", variable.buffer_offset, variable.data_size, (std::string)variable.type, variable.name);
        }
      }

      std::shared_ptr<PBRMaterialShader> pbr_material_shader = std::make_shared<PBRMaterialShader>();

      std::unique_ptr<Material> pbr_material = std::make_unique<Material>(pbr_material_shader);

      pbr_material->SetParameter<f32>("roughness", 0.25);
      pbr_material->SetParameter<f32>("metalness", 0.33);

      ZEPHYR_INFO("metalness={}", pbr_material->GetParameter<f32>("metalness"));
      ZEPHYR_INFO("roughness={}", pbr_material->GetParameter<f32>("roughness"));
    }
  }

  MainWindow::~MainWindow() {
    m_render_device->WaitIdle();
  }

  void MainWindow::OnFrame() {
    const uint frame_index = m_frame % m_frames_in_flight;
    const auto& command_buffer = m_render_command_buffers[frame_index];
    const auto& fence = m_fences[frame_index];
    const auto& bind_group = m_bind_groups[frame_index];

    fence->Wait();

    m_resource_uploader->BeginFrame();

    const auto& render_target = GetSwapChain()->AcquireNextRenderTarget();

    m_render_pass->SetClearColor(0, 0.02, 0.02, 0.02, 1.0);

    command_buffer->Begin(CommandBuffer::OneTimeSubmit::Yes);
    command_buffer->SetViewport(0, 0, m_width, m_height);
    command_buffer->BeginRenderPass(render_target.get(), m_render_pass.get());

    m_scene_root->Traverse([&](SceneNode* node) {
      node->GetTransform().UpdateLocal();
      node->GetTransform().UpdateWorld();
      return true;
    });

    const Mesh3D* current_mesh = nullptr;
    const Material* current_material = nullptr;
    GraphicsPipeline* current_pipeline = nullptr;

    m_scene_root->Traverse([&](SceneNode* node) {
      if(!node->IsVisible()) {
        return false;
      }

      if(node->HasComponent<MeshComponent>()) {
        const MeshComponent& mesh_component = node->GetComponent<MeshComponent>();
        const Mesh3D& mesh = *mesh_component.mesh;
        const Material& material = *mesh_component.material;
        const IndexBuffer*  ibo = mesh.GetIBO();
        const VertexBuffer* vbo = mesh.GetVBO();

        const bool new_material = &material != current_material;
        const bool new_mesh_layout = !current_mesh || mesh.GetLayoutKey() != current_mesh->GetLayoutKey();
        const bool new_mesh = &mesh != current_mesh;

        if(new_material || new_mesh_layout) {
          GraphicsPipeline* pipeline = m_material_pipeline_cache->GetGraphicsPipeline(Technique::Forward, &material, &mesh);

          command_buffer->BindPipeline(pipeline);
          command_buffer->BindBindGroup(PipelineBindPoint::Graphics, pipeline->GetLayout(), 0, bind_group.get());

          // Update bind group bindings
          {
            auto& sampler = m_texture->GetSampler();

            // problem: bind group can not be updated here (at least not more than once)
            // solution: possibly use vkCmdPushDescriptorSetKHR

            bind_group->Bind(
              0u,
              m_texture_cache->GetDeviceTexture(m_texture.get()),
              (bool)sampler ? m_sampler_cache->GetDeviceSampler(m_texture->GetSampler().get()) : m_render_device->DefaultLinearSampler(),
              Texture::Layout::ShaderReadOnly
            );

            bind_group->Bind(
              1u,
              m_texture_cache->GetDeviceTexture(m_texture_cube.get()),
              (bool)sampler ? m_sampler_cache->GetDeviceSampler(m_texture_cube->GetSampler().get()) : m_render_device->DefaultLinearSampler(),
              Texture::Layout::ShaderReadOnly
            );

            bind_group->Bind(32u, m_buffer_cache->GetDeviceBuffer(m_ubo.get()), BindingType::UniformBuffer);
          }

          current_material = &material;
          current_pipeline = pipeline;
        }

        // @todo: ideally do not embed the projection matrix on every draw call
        command_buffer->PushConstants(current_pipeline->GetLayout(), 0u, sizeof(Matrix4), &m_projection_matrix);
        command_buffer->PushConstants(current_pipeline->GetLayout(), sizeof(Matrix4), sizeof(Matrix4), &node->GetTransform().GetWorld());

        if(new_mesh) {
          if(ibo) {
            command_buffer->BindIndexBuffer(m_buffer_cache->GetDeviceBuffer(ibo), ibo->GetDataType());
          }
          command_buffer->BindVertexBuffers({{m_buffer_cache->GetDeviceBuffer(vbo)}});

          current_mesh = &mesh;
        }

        if(ibo) {
          command_buffer->DrawIndexed(ibo->GetNumberOfIndices());
        } else {
          command_buffer->Draw(vbo->GetNumberOfVertices());
        }
      }

      return true;
    });

    command_buffer->EndRenderPass();
    command_buffer->End();

    m_resource_uploader->FinalizeFrame();

    // @todo: remove TmpWaitForImageFullyRead() completely and use a semaphore instead.
    GetSwapChain()->TmpWaitForImageFullyRead();
    fence->Reset();
    m_render_device->GraphicsQueue()->Submit({{m_resource_uploader->GetCurrentCommandBuffer(), command_buffer.get()}}, fence.get());

    GetSwapChain()->Present();

    // Update the transform of the second cube
    const float angle = (f32)m_frame * 0.01f;
    auto& cube_b = m_scene_root->GetChildren()[0]->GetChildren()[0];
    cube_b->GetTransform().GetPosition() = Vector3{std::cos(angle), 0.0f, -std::sin(angle)} * 2.0f;
    cube_b->GetTransform().GetRotation().SetFromEuler(0.0f, angle, 0.0f);

    m_frame++;

    UpdateFramesPerSecondCounter();
  }

  void MainWindow::OnResize(int width, int height) {
    m_width = width;
    m_height = height;
    m_projection_matrix = Matrix4::PerspectiveVK(45.0f, (f32)width / (f32)height, 0.01f, 100.0f);
  }

  void MainWindow::Setup() {
    m_render_device = GetRenderDevice();

    // @todo: does it make sense to directly couple the frames in flight to the number of swapchain images?
    m_frames_in_flight = GetSwapChain()->GetNumberOfSwapChainImages();
    ZEPHYR_INFO("Renderer configured to have {} frame(s) in flight", m_frames_in_flight);

    CreateCommandPoolAndBuffers();
    CreateResourceUploader();
    CreateBufferCache();
    CreateTextureCache();
    CreateSamplerCache();
    CreateRenderPass();
    CreateMaterialPipelineCache();
    CreateFences();
    CreateBindGroups();
    TestShaderCompilation();
    CreateCubeMesh();
    CreateUniformBuffer();
    CreateTexture();
    CreateTextureCube();
    CreateScene();
  }

  void MainWindow::CreateCommandPoolAndBuffers() {
    m_command_pool = m_render_device->CreateGraphicsCommandPool(
      CommandPool::Usage::Transient | CommandPool::Usage::ResetCommandBuffer);

    for(uint i = 0; i < m_frames_in_flight; i++) {
      m_render_command_buffers.push_back(m_render_device->CreateCommandBuffer(m_command_pool));
    }
  }

  void MainWindow::CreateResourceUploader() {
    m_resource_uploader = std::make_shared<ResourceUploader>(m_render_device, m_command_pool, m_frames_in_flight);
  }

  void MainWindow::CreateBufferCache() {
    m_buffer_cache = std::make_shared<BufferCache>(m_render_device, m_resource_uploader);
  }

  void MainWindow::CreateTextureCache() {
    m_texture_cache = std::make_shared<TextureCache>(m_render_device, m_resource_uploader);
  }

  void MainWindow::CreateSamplerCache() {
    m_sampler_cache = std::make_shared<SamplerCache>(m_render_device);
  }

  void MainWindow::CreateRenderPass() {
    auto builder = m_render_device->CreateRenderPassBuilder();

    builder->SetColorAttachmentFormat(0, Texture::Format::B8G8R8A8_SRGB);
    builder->SetColorAttachmentSrcLayout(0, Texture::Layout::Undefined, std::nullopt);
    builder->SetColorAttachmentDstLayout(0, Texture::Layout::PresentSrc, std::nullopt);

    builder->SetDepthAttachmentFormat(Texture::Format::DEPTH_U16);
    builder->SetDepthAttachmentSrcLayout(Texture::Layout::Undefined, std::nullopt);
    builder->SetDepthAttachmentDstLayout(Texture::Layout::DepthStencilAttachment, std::nullopt);

    m_render_pass = builder->Build();
  }

  void MainWindow::CreateMaterialPipelineCache() {
    m_material_pipeline_cache = std::make_shared<MaterialPipelineCache>(m_render_device);

    m_material_pipeline_cache->RegisterTechnique(Technique::Forward, m_render_pass);
  }

  void MainWindow::CreateFences() {
    for(uint i = 0; i < m_frames_in_flight; i++) {
      m_fences.push_back(m_render_device->CreateFence(Fence::CreateSignalled::Yes));
    }
  }

  void MainWindow::CreateBindGroups() {
    // @todo: deduplicate bind group layout creation

    std::array<BindGroupLayout::Entry, 33> bindings;

    for(u32 binding = 0; binding < 32u; binding++) {
      bindings[binding] = {
        .binding = binding,
        .type = BindingType::ImageWithSampler,
        .stages = ShaderStage::All
      };
    }

    bindings[32] = {
      .binding = 32u,
      .type = BindingType::UniformBuffer,
      .stages = ShaderStage::All
    };

    m_bind_group_layout = m_render_device->CreateBindGroupLayout(bindings);

    for(uint i = 0; i < m_frames_in_flight; i++) {
      m_bind_groups.push_back(m_render_device->CreateBindGroup(m_bind_group_layout));
    }
  }

  void MainWindow::CreateCubeMesh() {
    /**
     *   4-------5
     *  /|      /|
     * 0-------1 |
     * | 6-----|-7
     * |/      |/
     * 2-------3
     */
    static const float k_vertices[] = {
      //    | POSITION       | COLOR         | UV

      // front face
      /*0*/ -1.0, -1.0,  1.0,  1.0, 0.0, 0.0,  0.0, 0.0,
      /*1*/  1.0, -1.0,  1.0,  0.0, 1.0, 0.0,  1.0, 0.0,
      /*2*/ -1.0,  1.0,  1.0,  0.0, 0.0, 1.0,  0.0, 1.0,
      /*3*/  1.0,  1.0,  1.0,  0.5, 0.5, 0.5,  1.0, 1.0,

      // back face
      /*4*/ -1.0, -1.0, -1.0,  1.0, 1.0, 0.0,  0.0, 0.0,
      /*5*/  1.0, -1.0, -1.0,  1.0, 0.0, 1.0,  1.0, 0.0,
      /*6*/ -1.0,  1.0, -1.0,  0.0, 1.0, 1.0,  0.0, 1.0,
      /*7*/  1.0,  1.0, -1.0,  1.0, 1.0, 1.0,  1.0, 1.0
    };
    
    static const u16 k_indices[] = {
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

    std::shared_ptr<Mesh3D> mesh = std::make_shared<Mesh3D>(8, 36, Mesh3D::Attribute::Color0 | Mesh3D::Attribute::UV0);

    for(size_t i = 0u; i < 8u; i++) {
      mesh->SetPosition(i, Vector3{k_vertices[i * 8 + 0], k_vertices[i * 8 + 1], k_vertices[i * 8 + 2]});
      mesh->SetColor0(i, Vector3{k_vertices[i * 8 + 3], k_vertices[i * 8 + 4], k_vertices[i * 8 + 5]});
      mesh->SetUV0(i, Vector2{k_vertices[i * 8 + 6], k_vertices[i * 8 + 7]});
    }

    for(size_t i = 0u; i < 36u; i++) {
      mesh->SetIndex(i, k_indices[i]);
    }

    m_cube_mesh = std::move(mesh);
  }

  void MainWindow::CreateUniformBuffer() {
    Vector4 color{1.0f, 0.0f, 0.0f, 0.0f};

    m_ubo = std::make_unique<UniformBuffer>(std::span{(const u8*)&color, sizeof(color)});
  }

  void MainWindow::CreateTexture() {
    int width;
    int height;
    int channels_in_file;

    const u8* texture_data = stbi_load("test.jpg", &width, &height, &channels_in_file, 4);

    m_texture = std::make_unique<Texture2D>(width, height, Texture2D::Format::RGBA, Texture2D::DataType::UnsignedByte, Texture2D::ColorSpace::SRGB);

    std::memcpy(m_texture->Data(), texture_data, m_texture->Size());
  }

  void MainWindow::CreateTextureCube() {
    int face_size;

    /*const u8* face_data[6];

    const char* paths[6] { "env/px.png", "env/nx.png", "env/py.png", "env/ny.png", "env/pz.png", "env/nz.png" };

    for(int i = 0; i < 6; i++) {
      int width;
      int height;
      int channels_in_file;

      face_data[i] = stbi_load(paths[i], &width, &height, &channels_in_file, 4);

      if(face_data[i] == nullptr) {
        ZEPHYR_PANIC("Failed to load cube map face: {}", paths[i])
      }

      if(i == 0) {
        face_size = width;
      }

      if(width != height || width != face_size) {
        ZEPHYR_PANIC("Bad cube map texture size");
      }
    */
    face_size = 128;

    m_texture_cube = std::make_unique<TextureCube>(
      face_size, TextureCube::Format::RGBA, TextureCube::DataType::UnsignedByte, TextureCube::ColorSpace::SRGB);

    /*for(int i = 0; i < 6; i++) {
      std::memcpy(m_texture_cube->Data<u8>((TextureCube::Face)i), face_data[i], m_texture_cube->Size()/6);
    }*/
  }

  void MainWindow::CreateScene() {
    m_scene_root = std::make_unique<SceneNode>();

    std::shared_ptr<Material> pbr_material = std::make_shared<Material>(std::make_shared<PBRMaterialShader>());

    SceneNode* cube_a = m_scene_root->CreateChild("Cube A");
    cube_a->CreateComponent<MeshComponent>(m_cube_mesh, pbr_material);
    cube_a->GetTransform().GetPosition() = Vector3{0.0f, 0.0f, -5.0f};

    SceneNode* cube_b = cube_a->CreateChild("Cube B");
    cube_b->CreateComponent<MeshComponent>(m_cube_mesh, pbr_material);
    cube_b->GetTransform().GetScale() = Vector3{0.25f, 0.25f, 0.25f};
  }

  void MainWindow::UpdateFramesPerSecondCounter() {
    const auto time_point_now = std::chrono::steady_clock::now();

    const auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      time_point_now - m_time_point_last_update).count();

    m_fps_counter++;

    if(time_elapsed >= 1000) {
      const f32 fps = (f32)m_fps_counter * 1000.0f / (f32)time_elapsed;
      SetWindowTitle(fmt::format("Zephyr Runtime ({} fps)", fps));
      m_fps_counter = 0;
      m_time_point_last_update = std::chrono::steady_clock::now();
    }
  }

  static glsl_include_result_t* glslang_system_includer(void* ctx, const char* header_name, const char* includer_name, size_t include_depth) {
    static const std::string header = R"(
void foo() {
}
)";

    glsl_include_result_t* result = new glsl_include_result_t();
    result->header_name = header_name; // @todo: is that valid? what is this used for? avoiding multiple includes?
    result->header_data = header.data();
    result->header_length = header.size();
    return result;
  }

  static int glslang_free_include_result(void* ctx, glsl_include_result_t* result) {
    // @todo
    return 0; // @todo: should this return 0 or 1?
  }

  void MainWindow::TestShaderCompilation() {
    const auto ReadAsString = [](const std::string& path) {
      std::ifstream file{path, std::ios::binary};

      if(!file.is_open()) {
        ZEPHYR_PANIC("Failed to open file for reading: {}", path);
      }

      size_t file_size;
      file.seekg(0, std::ios::end);
      file_size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::string text;
      text.reserve(file_size);
      text.assign(std::istreambuf_iterator<char>{file}, {});
      file.close();
      return text;
    };

    const auto CompileShader = [](const std::string& shader_code, glslang_stage_t shader_stage) {
      const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = shader_stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_2,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_5,
        .code = shader_code.data(),
        .default_version = 450,
        .default_profile = GLSLANG_CORE_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource(),
        .callbacks = {
          .include_system = &glslang_system_includer,
          .include_local = nullptr,
          .free_include_result = &glslang_free_include_result
        }
      };

      glslang_initialize_process();

      glslang_shader_t* shader = glslang_shader_create(&input);

      if(!glslang_shader_preprocess(shader, &input)) {
        ZEPHYR_PANIC("GLSL preprocessing failed:\n{}\n{}\n{}",
          glslang_shader_get_info_log(shader), glslang_shader_get_info_debug_log(shader), shader_code);
        //glslang_shader_delete(shader);
      }

      if(!glslang_shader_parse(shader, &input)) {
        ZEPHYR_PANIC("GLSL parsing failed:\n{}\n{}\n{}",
          glslang_shader_get_info_log(shader), glslang_shader_get_info_debug_log(shader),
          glslang_shader_get_preprocessed_code(shader));
        //glslang_shader_delete(shader);
      }

      glslang_program_t* program = glslang_program_create();

      glslang_program_add_shader(program, shader);

      if(!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        ZEPHYR_PANIC("GLSL linking failed:\n{}\n{}",
          glslang_program_get_info_log(program), glslang_program_get_info_debug_log(program));
        //glslang_program_delete(program);
        //glslang_shader_delete(shader);
      }

      glslang_program_SPIRV_generate(program, shader_stage);
    };

    std::unique_ptr<Material> pbr_material = std::make_unique<Material>(std::make_shared<PBRMaterialShader>());

    const std::string uniform_block_template = R"(
// TODO: apparently this extension is required to use std430 layout with uniform buffers:
// https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_scalar_block_layout.txt
#extension GL_EXT_scalar_block_layout : require

layout(set = 0, binding = 0, std430) uniform MaterialParams {{
{}
}};
)";

    std::string uniform_declarations{};

    const MaterialShader& shader = pbr_material->GetShader();

    for(auto variable : shader.GetParameterBufferLayout().GetVariables()) {
      uniform_declarations += fmt::format("  {} {};\n", (std::string)variable.type, variable.name);
    }

    const std::string uniform_block = fmt::format(fmt::runtime(uniform_block_template), uniform_declarations);

    // @todo: think how we want to treat material parameters for different technique, some of which may be generic

    const TechniquePass& technique_pass = shader.GetTechniquePass(Technique::Forward).value();

    CompileShader(uniform_block + ReadAsString(technique_pass.frag_shader_path), GLSLANG_STAGE_FRAGMENT);

  }

} // namespace zephyr

#include <filesystem>
#include <iostream>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <PixieRendering/Materials/PBRMaterial.h>
#include <PixieRendering/PixieRendering.h>
#include <PixieRendering/Resources/Camera.h>

#include <PixieApplication/Time/ApplicationTime.h>
#include <PixieUIApplication/PixieUIApplication.h>
#include <PixieUIApplication/Windows/DemoWindow.h>
#include <PixieUIApplication/Windows/TextureDisplayWindow.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ufbx.h>

using namespace PixieRenderer;

const char* vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in ivec4 boneIDs; 
layout(location = 4) in vec4 boneWeights; 

layout(location = 0) out vec2 TexCoord;

layout(set = 0, binding = 1, std140) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

layout(set = 0, binding = 0, std140) uniform ModelUBO {
    mat4 model;
} modelData;

void main()
{
    gl_Position = camera.projection * camera.view * modelData.model * vec4(aPos * 0.25, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 450

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 2) uniform sampler2D texSampler;

void main()
{
    FragColor = texture(texSampler, TexCoord);
}
)";

std::string GetDirectory(const std::string& path) {
	size_t pos = path.find_last_of("/\\");
	if (pos == std::string::npos) {
		return std::string();
	}
	return path.substr(0, pos + 1);
}

std::string UfbxStringToStd(const ufbx_string& s) {
	if (!s.data) {
		return std::string();
	}
	return std::string(s.data, s.length);
}

class SponzaSceneApp : public PixieApp::PixieUIApplication {
  public:
	std::vector<MeshHandle> m_meshes;
	std::vector<PBRMaterial> m_materials;
	std::vector<MaterialHandle> m_materialHandles;
	std::vector<std::pair<uint32_t, uint32_t>> m_materialAssignments;

	SponzaSceneApp(const char* path, const std::string& scenePath)
	    : PixieUIApplication("Simple scene", { 1280, 720 }, RenderAPI::Vulkan, true) {
		m_frameBuffer = m_renderer->CreateFrameBuffer({ 1280, 720 }, TextureFormat::RGBA32f);

		m_ui->AddWindow(new PixieUI::DemoWindow(m_ui, m_renderer));
		m_ui->AddWindow(new PixieUI::TextureDisplayWindow(m_ui, m_renderer, m_frameBuffer));

		std::filesystem::path appPath = std::filesystem::path(path);

		LoadScene(scenePath);

		glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, -5.0f);
		glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
		m_camera.view = glm::lookAt(cameraPosition, center, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void LoadScene(const std::string& path) {

		//entt::registry registry;

		//entt::entity player = registry.create();
		//registry.emplace<TransformComponent>(player, 10.0f, 20.0f);
		//registry.emplace<SpriteComponent>(player, "player_sheet.png", 0xFF0000FF); // Red player

		//for (int i = 0; i (tree, i * 50.0f, 0.0f);
		//	registry.emplace<SpriteComponent>(tree, "tree.png", 0x00FF00FF); // Green tree
		//}

		//entt::entity camera = registry.create();
		//registry.emplace<TransformComponent>(camera, 0.0f, 0.0f);
		//registry.emplace<CameraComponent>(camera, 1.5f);


		ufbx_error error;
		ufbx_scene* scene = ufbx_load_file(path.c_str(), nullptr, &error);
		if (!scene) {
			std::cerr << "Failed to load FBX: "
			          << (error.description.data ? error.description.data : "unknown error")
			          << std::endl;
			exit(1);
		}

		m_materials.reserve(scene->materials.count);
		for (size_t i = 0; i < scene->materials.count; i++) {
			ufbx_material* um = scene->materials.data[i];
			PBRMaterial mat;
			mat.name = UfbxStringToStd(um->name);

			const ufbx_vec4 base = um->pbr.base_color.value_vec4;
			mat.SetAlbedo(glm::vec3(base.x, base.y, base.z));

			mat.SetMetallic(static_cast<float>(um->pbr.metalness.value_real));
			mat.SetRoughness(static_cast<float>(um->pbr.roughness.value_real));

			if (um->pbr.base_color.texture) {
				std::string texturePath = UfbxStringToStd(um->pbr.base_color.texture->filename);
				TextureHandle handle = LoadTexture(texturePath);
				mat.SetAlbedoTexture(handle);
			}
			if (um->pbr.normal_map.texture) {
				std::string texturePath = UfbxStringToStd(um->pbr.normal_map.texture->filename);
				TextureHandle handle = LoadTexture(texturePath);
				mat.SetAlbedoTexture(handle);
			}
			if (um->pbr.metalness.texture) {
				std::string texturePath = UfbxStringToStd(um->pbr.metalness.texture->filename);
				TextureHandle handle = LoadTexture(texturePath);
				mat.SetAlbedoTexture(handle);
			}
			if (um->pbr.roughness.texture) {
				std::string texturePath = UfbxStringToStd(um->pbr.roughness.texture->filename);
				TextureHandle handle = LoadTexture(texturePath);
				mat.SetAlbedoTexture(handle);
			}

			m_materials.push_back(mat);

			MaterialHandle materialHandle = m_renderer->CreateMaterial(&mat);
		}

		for (size_t ni = 0; ni < scene->nodes.count; ni++) {
			ufbx_node* node = scene->nodes.data[ni];
			if (!node->mesh)
				continue;

			ufbx_mesh* umesh = node->mesh;
			if (umesh->num_faces == 0 || umesh->num_triangles == 0)
				continue;

			const ufbx_matrix& world = node->geometry_to_world;

			std::unordered_map<uint32_t, Mesh*> meshesByMaterial;

			for (size_t fi = 0; fi < umesh->num_faces; fi++) {
				const ufbx_face face = umesh->faces.data[fi];

				uint32_t matId = 0;
				if (umesh->face_material.data && fi < umesh->face_material.count) {
					matId = umesh->face_material.data[fi];
				}

				Mesh*& mesh = meshesByMaterial[matId];
				if (!mesh) {
					mesh = new Mesh();
				}

				for (uint32_t i = 2; i < face.num_indices; i++) {
					const uint32_t tri[3] = { face.index_begin,
						                      face.index_begin + i - 1,
						                      face.index_begin + i };

					for (int j = 0; j < 3; j++) {
						const uint32_t vi = tri[j];

						Vertex vertex{};

						ufbx_vec3 p = ufbx_get_vertex_vec3(&umesh->vertex_position, vi);
						p = ufbx_transform_position(&world, p);
						vertex.position = glm::vec3(p.x, p.y, p.z);

						if (umesh->vertex_normal.exists) {
							ufbx_vec3 n = ufbx_get_vertex_vec3(&umesh->vertex_normal, vi);
							n = ufbx_transform_direction(&world, n);
							glm::vec3 gn(n.x, n.y, n.z);
							const float len = glm::length(gn);
							vertex.normal = (len > 1e-8f) ? gn / len : glm::vec3(0.0f, 0.0f, 1.0f);
						}

						if (umesh->vertex_uv.exists) {
							ufbx_vec2 uv = ufbx_get_vertex_vec2(&umesh->vertex_uv, vi);
							vertex.uv = glm::vec2(uv.x, uv.y);
						}

						mesh->vertexes.push_back(vertex);
						mesh->indexes.push_back(static_cast<int32_t>(mesh->vertexes.size() - 1));
					}
				}
			}
		}

		ufbx_free_scene(scene);

		std::cout << "FBX loaded: " << m_meshes.size() << " mesh(es), " << m_materials.size()
		          << " material(s)." << std::endl;
	}

	TextureHandle LoadTexture(const std::string& filePath) {
		int width, height, channels;
		stbi_uc* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);

		if (!data) {
			std::cout << "failed to load texture\n";
			exit(2);
		}

		Image2D image;
		image.resolution = glm::ivec2(width, height);
		image.format = TextureFormat::RGBA8;
		image.pixels.resize(width * height * 4);
		memcpy(image.pixels.data(), data, image.pixels.size());

		stbi_image_free(data);

		return m_renderer->CreateTexture(&image);
	}

	void BeforeDrawFrame() override {
		m_ui->OnBeforeDrawFrame();

		glm::mat4 model = glm::mat4(1.0f);
		m_angle += PixieApp::Time::deltaTime * 0.5f;
		model = glm::rotate(model, m_angle, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, m_angle * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, m_angle * 0.3f, glm::vec3(0.0f, 0.0f, 1.0f));
		m_renderer->LoadUniformBuffer(m_materialHandle, "ModelUBO", &model, sizeof(glm::mat4));

		float aspect = static_cast<float>(m_window->GetResolution().x) /
		               m_window->GetResolution().y;
		m_camera.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
		m_renderer->LoadUniformBuffer(m_materialHandle, "CameraUBO", &m_camera, sizeof(Camera));

		m_renderer->BindTexture(m_materialHandle, "texSampler", m_texture, 0);

		m_renderer->BeginRenderPass(m_frameBuffer);
		m_renderer->DrawMesh(m_meshHandle, m_materialHandle);
		m_renderer->EndRenderPass();
	}

  private:
	FrameBufferHandle m_frameBuffer;
	Camera m_camera;
	float m_angle = 0.0f;
};

int32_t main(int argc, char** argv) {
	SponzaSceneApp* app = new SponzaSceneApp(
	    argv[0],
	    "C:/Repos/PixieRendering/assets/main_sponza/NewSponza_Main_Yup_003.fbx"
	);

	app->Start();

	delete app;

	return 0;
}

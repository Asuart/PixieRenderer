#include <algorithm>
#include <filesystem>
#include <iostream>
#include <unordered_map>

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
#include <PixieUIApplication/Windows/ApplicationStatsWindow.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ufbx.h>

#include <entt/entt.hpp>

using namespace PixieRenderer;

static std::string UfbxStringToStd(const ufbx_string& s) {
	if (!s.data)
		return std::string();
	return std::string(s.data, s.length);
}

static glm::mat4 UfbxMatrixToGlm(const ufbx_matrix& m) {
	glm::mat4 result(1.0f);
	result[0] = glm::vec4(m.cols[0].x, m.cols[0].y, m.cols[0].z, 0.0f);
	result[1] = glm::vec4(m.cols[1].x, m.cols[1].y, m.cols[1].z, 0.0f);
	result[2] = glm::vec4(m.cols[2].x, m.cols[2].y, m.cols[2].z, 0.0f);

	const ufbx_vec3 t = ufbx_transform_position(&m, ufbx_vec3{ 0.0, 0.0, 0.0 });
	result[3] = glm::vec4(t.x, t.y, t.z, 1.0f);

	return result;
}

struct TransformComponent {
	glm::mat4 transform = glm::mat4(1.0f);
};

struct MeshComponent {
	MeshHandle mesh{};
};

struct MaterialComponent {
	MaterialHandle materialHandle{};
	PBRMaterial* material = nullptr;
};

struct CameraComponent {
	Camera camera;
};

class SponzaSceneApp : public PixieApp::PixieUIApplication {
  public:
	std::vector<MeshHandle> m_meshes;
	std::vector<PBRMaterial> m_materials;
	std::vector<MaterialHandle> m_materialHandles;

	entt::registry m_registry;
	entt::entity m_cameraEntity = entt::null;

	SponzaSceneApp(const std::string& scenePath)
	    : PixieUIApplication("Sponza scene", { 1280, 720 }, RenderAPI::Vulkan, true) {
		m_frameBuffer = m_renderer->CreateFrameBuffer({ 1280, 720 }, TextureFormat::RGBA32f);

		m_ui->AddWindow(new PixieUI::DemoWindow(m_ui, m_renderer));
		m_ui->AddWindow(new PixieUI::ApplicationStatsWindow(m_ui, m_renderer));
		m_ui->AddWindow(new PixieUI::TextureDisplayWindow(m_ui, m_renderer, m_frameBuffer));

		LoadScene(scenePath);
	}

	void LoadScene(const std::string& path) {
		glm::vec3 cameraPosition = glm::vec3(0.0f, 1.5f, -4.0f);
		glm::vec3 center = glm::vec3(0.0f, 1.0f, 0.0f);

		Camera camera;
		camera.view = glm::lookAt(cameraPosition, center, glm::vec3(0.0f, 1.0f, 0.0f));
		camera.projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 5000.0f);

		m_cameraEntity = m_registry.create();
		m_registry.emplace<CameraComponent>(m_cameraEntity, CameraComponent{ camera });

		ufbx_error error;
		ufbx_scene* scene = ufbx_load_file(path.c_str(), nullptr, &error);
		if (!scene) {
			std::cerr << "Failed to load FBX: "
			          << (error.description.data ? error.description.data : "unknown error")
			          << std::endl;
			std::exit(1);
		}

		m_materials.reserve(scene->materials.count);
		m_materialHandles.reserve(scene->materials.count);

		for (size_t i = 0; i < scene->materials.count; i++) {
			ufbx_material* um = scene->materials.data[i];

			PBRMaterial mat;
			mat.name = UfbxStringToStd(um->name);

			const ufbx_vec4 base = um->pbr.base_color.value_vec4;
			mat.SetAlbedo(glm::vec3(base.x, base.y, base.z));
			mat.SetMetallic(static_cast<float>(um->pbr.metalness.value_real));
			mat.SetRoughness(static_cast<float>(um->pbr.roughness.value_real));

			if (um->pbr.base_color.texture) {
				std::string texPath = UfbxStringToStd(um->pbr.base_color.texture->filename);
				mat.SetAlbedoTexture(LoadTexture(texPath));
			} else {
				mat.SetAlbedoTexture(CreateFallbackTexture());
			}
			if (um->pbr.normal_map.texture) {
				std::string texPath = UfbxStringToStd(um->pbr.normal_map.texture->filename);
				mat.SetNormalTexture(LoadTexture(texPath));
			}
			if (um->pbr.metalness.texture) {
				std::string texPath = UfbxStringToStd(um->pbr.metalness.texture->filename);
				mat.SetMetallicTexture(LoadTexture(texPath));
			}
			if (um->pbr.roughness.texture) {
				std::string texPath = UfbxStringToStd(um->pbr.roughness.texture->filename);
				mat.SetRoughnessTexture(LoadTexture(texPath));
			}

			m_materials.push_back(std::move(mat));
			m_materialHandles.push_back(m_renderer->CreateMaterial(&m_materials.back()));
		}

		for (size_t ni = 0; ni < scene->nodes.count; ni++) {
			ufbx_node* node = scene->nodes.data[ni];
			if (!node->mesh)
				continue;
			if (!node->visible)
				continue;

			ufbx_mesh* umesh = node->mesh;
			if (umesh->num_faces == 0 || umesh->num_triangles == 0)
				continue;

			const glm::mat4 worldMatrix = UfbxMatrixToGlm(node->geometry_to_world);

			std::unordered_map<uint32_t, Mesh*> meshesByMaterial;

			for (size_t fi = 0; fi < umesh->num_faces; fi++) {
				const ufbx_face face = umesh->faces.data[fi];

				uint32_t matId = 0;
				if (umesh->face_material.data && fi < umesh->face_material.count) {
					matId = umesh->face_material.data[fi];
				}

				Mesh*& mesh = meshesByMaterial[matId];
				if (!mesh)
					mesh = new Mesh();

				for (uint32_t i = 2; i < face.num_indices; i++) {
					const uint32_t tri[3] = { face.index_begin,
						                      face.index_begin + i - 1,
						                      face.index_begin + i };

					for (int j = 0; j < 3; j++) {
						const uint32_t vi = tri[j];

						Vertex vertex{};

						ufbx_vec3 p = ufbx_get_vertex_vec3(&umesh->vertex_position, vi);
						vertex.position = glm::vec3(p.x, p.y, p.z);

						if (umesh->vertex_normal.exists) {
							ufbx_vec3 n = ufbx_get_vertex_vec3(&umesh->vertex_normal, vi);
							glm::vec3 gn(n.x, n.y, n.z);
							const float len = glm::length(gn);
							vertex.normal = (len > 1e-8f) ? gn / len : glm::vec3(0.0f, 1.0f, 0.0f);
						} else {
							vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
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

			for (auto& [matId, mesh] : meshesByMaterial) {
				if (!mesh || mesh->vertexes.empty()) {
					delete mesh;
					continue;
				}

				MeshHandle meshHandle = m_renderer->CreateMesh(mesh);
				m_meshes.push_back(meshHandle);
				delete mesh;

				MaterialHandle matHandle{};
				PBRMaterial* material = nullptr;
				if (!m_materialHandles.empty()) {
					size_t safeId = std::min<size_t>(matId, m_materialHandles.size() - 1);
					matHandle = m_materialHandles[safeId];
					material = &m_materials[safeId];
					material->m_handle = matHandle;
				}

				entt::entity e = m_registry.create();
				m_registry.emplace<TransformComponent>(e, TransformComponent{ worldMatrix });
				m_registry.emplace<MeshComponent>(e, MeshComponent{ meshHandle });
				m_registry.emplace<MaterialComponent>(e, MaterialComponent{ matHandle, material });
			}
		}

		ufbx_free_scene(scene);

		std::cout << "FBX loaded: " << m_meshes.size() << " mesh(es), " << m_materials.size()
		          << " material(s)." << std::endl;
		std::cout << "Mesh entities created: " << m_registry.view<MeshComponent>().size()
		          << std::endl;
	}

	TextureHandle CreateFallbackTexture() {
		static TextureHandle fallbackHandle;
		if (fallbackHandle) {
			return fallbackHandle;
		}

		Image2D fallback;
		fallback.resolution = glm::ivec2(1, 1);
		fallback.format = TextureFormat::RGBA8;
		fallback.pixels = { 255, 255, 255, 255 };
		fallbackHandle = m_renderer->CreateTexture(&fallback);

		return fallbackHandle;
	}

	TextureHandle LoadTexture(const std::string& filePath) {
		int width = 0, height = 0, channels = 0;
		stbi_uc* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);

		if (!data) {
			std::cout << "Failed to load texture: " << filePath << " — using 1x1 white fallback.\n";
			return CreateFallbackTexture();
		}

		Image2D image;
		image.resolution = glm::ivec2(width, height);
		image.format = TextureFormat::RGBA8;
		image.pixels.resize(static_cast<size_t>(width) * height * 4);
		std::memcpy(image.pixels.data(), data, image.pixels.size());

		stbi_image_free(data);
		return m_renderer->CreateTexture(&image);
	}

	void BeforeDrawFrame() override {
		m_ui->OnBeforeDrawFrame();

		auto& camComp = m_registry.get<CameraComponent>(m_cameraEntity);

		const auto res = m_window->GetResolution();
		const float aspect = static_cast<float>(res.x) / static_cast<float>(res.y);
		camComp.camera.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 5000.0f);

		const float t = static_cast<float>(glfwGetTime());
		constexpr float kSpeed = 0.5f;
		constexpr float kRadius = 6.0f;
		const glm::vec3 center(0.0f, 1.0f, 0.0f);

		const float angle = t * kSpeed;
		const glm::vec3 cameraPosition(
		    center.x + kRadius * std::sin(angle),
		    center.y + 1.5f,
		    center.z + kRadius * std::cos(angle)
		);

		camComp.camera.view = glm::lookAt(cameraPosition, center, glm::vec3(0.0f, 1.0f, 0.0f));

		const glm::vec4 camPos = glm::vec4(glm::vec3(glm::inverse(camComp.camera.view)[3]), 1.0f);

		for (size_t i = 0; i < m_materials.size(); i++) {
			m_materials[i].Bind(m_renderer);

			m_renderer->LoadUniformBuffer(
			    m_materials[i].m_handle,
			    "CameraUBO",
			    &camComp.camera,
			    sizeof(Camera)
			);

			m_renderer->LoadUniformBuffer(
			    m_materials[i].m_handle,
			    "CameraPosition",
			    &camPos,
			    sizeof(glm::vec4)
			);
		}

		m_renderer->BeginRenderPass(m_frameBuffer);

		auto view = m_registry.view<TransformComponent, MeshComponent, MaterialComponent>();
		for (auto [entity, transform, meshComp, matComp] : view.each()) {
			m_renderer->DrawMesh(
			    meshComp.mesh,
			    matComp.materialHandle,
			    &transform.transform,
			    sizeof(glm::mat4)
			);
		}

		m_renderer->EndRenderPass();
	}

  private:
	FrameBufferHandle m_frameBuffer;
};

int32_t main(int argc, char** argv) {
	SponzaSceneApp* app = new SponzaSceneApp(
	    "C:/Repos/PixieRendering/assets/main_sponza/NewSponza_Main_Yup_003.fbx"
	);

	app->Start();

	delete app;

	return 0;
}

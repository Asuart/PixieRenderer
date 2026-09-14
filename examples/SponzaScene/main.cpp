#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <unordered_map>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <PixieRendering/Camera/Camera.h>
#include <PixieRendering/Material/MeshIslandsMaterial.h>
#include <PixieRendering/Material/PBRMaterial.h>
#include <PixieRendering/PixieRendering.h>

#include <PixieApplication/Time/ApplicationTime.h>
#include <PixieApplication/UserInput/UserInput.h>

#include <PixieUIApplication/PixieUIApplication.h>
#include <PixieUIApplication/Windows/ApplicationStatsWindow.h>
#include <PixieUIApplication/Windows/DemoWindow.h>
#include <PixieUIApplication/Windows/TextureDisplayWindow.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <ufbx.h>

#include <entt/entt.hpp>

using namespace PixieRenderer;
using namespace PixieApp;

static constexpr int kMaxTextureSize = 2048;

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

struct MeshLoadStats {
	size_t uniqueMeshes = 0;
	size_t totalVertices = 0;
	size_t totalIndices = 0;
	size_t totalTriangles = 0;
	size_t totalBytes = 0;
};

struct TextureLoadStats {
	size_t uniqueTextures = 0;
	size_t loadedFiles = 0;
	size_t failedLoads = 0;
	size_t fallbackTextures = 0;
	size_t totalBytes = 0;
	size_t maxWidth = 0;
	size_t maxHeight = 0;
};

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

	glm::vec3 position = glm::vec3(0.0f, 1.5f, -4.0f);
	float yaw = -90.0f;
	float pitch = 0.0f;
	float moveSpeed = 30.0f;
	float mouseSensitivity = 0.2f;
	bool cursorCaptured = true;
	bool lookEnabled = true;
};

class SponzaSceneApp : public PixieApp::PixieUIApplication {
  public:
	std::vector<MeshHandle> m_meshes;
	std::vector<PBRMaterial> m_materials;
	std::vector<MaterialHandle> m_materialHandles;
	std::unordered_map<std::string, TextureHandle> m_textureCache;

	entt::registry m_registry;
	entt::entity m_cameraEntity = entt::null;

	MeshLoadStats m_meshStats;
	TextureLoadStats m_textureStats;

	MeshIslandsMaterial m_meshIslandsMaterial;
	MaterialHandle m_meshIslandsMaterialHandle;
	bool m_showmeshIslands = true;

	SponzaSceneApp(const std::string& scenePath)
	    : PixieUIApplication("Sponza scene", { 1280, 720 }, RenderAPI::Vulkan, true) {
		m_frameBuffer = m_renderer->CreateFrameBuffer({ 1280, 720 }, TextureFormat::RGBA32f);

		m_ui->AddWindow(new PixieUI::DemoWindow(m_ui, m_renderer));
		m_ui->AddWindow(new PixieUI::ApplicationStatsWindow(m_ui, m_renderer));
		m_ui->AddWindow(new PixieUI::TextureDisplayWindow(m_ui, m_renderer, m_frameBuffer));

		m_meshIslandsMaterialHandle = m_renderer->CreateMaterial(&m_meshIslandsMaterial);

		LoadScene(scenePath);
	}

	void LoadScene(const std::string& path) {
		CameraComponent camComp;
		camComp.position = glm::vec3(0.0f, 1.5f, -4.0f);
		camComp.yaw = -90.0f;
		camComp.pitch = -5.0f;
		camComp.camera
		    .projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 5000.0f);

		m_cameraEntity = m_registry.create();
		m_registry.emplace<CameraComponent>(m_cameraEntity, camComp);

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

			const ufbx_vec4 base = um->pbr.base_color.value_vec4;
			mat.SetAlbedo(glm::vec3(base.x, base.y, base.z));
			mat.SetMetallic(static_cast<float>(um->pbr.metalness.value_real));
			mat.SetRoughness(static_cast<float>(um->pbr.roughness.value_real));

			if (um->pbr.base_color.texture) {
				std::string texPath = UfbxStringToStd(um->pbr.base_color.texture->filename);
				mat.SetAlbedoTexture(LoadTextureCached(texPath, true));
			} else {
				mat.SetAlbedoTexture(CreateFallbackTexture());
			}
			if (um->pbr.normal_map.texture) {
				std::string texPath = UfbxStringToStd(um->pbr.normal_map.texture->filename);
				mat.SetNormalTexture(LoadTextureCached(texPath, false));
			} else {
				mat.SetNormalTexture(CreateFallbackNormalTexture());
			}
			if (um->pbr.metalness.texture) {
				std::string texPath = UfbxStringToStd(um->pbr.metalness.texture->filename);
				mat.SetMetallicTexture(LoadTextureCached(texPath, false));
			} else {
				mat.SetMetallicTexture(CreateFallbackMetallicTexture());
			}
			if (um->pbr.roughness.texture) {
				std::string texPath = UfbxStringToStd(um->pbr.roughness.texture->filename);
				mat.SetRoughnessTexture(LoadTextureCached(texPath, false));
			} else {
				mat.SetRoughnessTexture(CreateFallbackRoughnessTexture());
			}

			m_materials.push_back(std::move(mat));
			m_materialHandles.push_back(m_renderer->CreateMaterial(&m_materials.back()));
		}

		std::unordered_map<const ufbx_mesh*, std::unordered_map<uint32_t, MeshHandle>> meshCache;

		for (size_t ni = 0; ni < scene->nodes.count; ni++) {
			ufbx_node* node = scene->nodes.data[ni];

			std::string nodeName = UfbxStringToStd(node->name);
			if (nodeName.contains("decal")) {
				continue;
			}

			if (!node->mesh)
				continue;
			if (!node->visible)
				continue;

			ufbx_mesh* umesh = node->mesh;
			if (umesh->num_faces == 0 || umesh->num_triangles == 0)
				continue;

			const glm::mat4 worldMatrix = UfbxMatrixToGlm(node->geometry_to_world);

			auto& cacheForMesh = meshCache[umesh];

			std::vector<uint32_t> usedMaterials;
			{
				std::unordered_map<uint32_t, bool> seen;
				seen.reserve(umesh->num_faces);
				for (size_t fi = 0; fi < umesh->num_faces; fi++) {
					uint32_t matId = 0;
					if (umesh->face_material.data && fi < umesh->face_material.count) {
						matId = umesh->face_material.data[fi];
					}
					if (seen.emplace(matId, true).second) {
						usedMaterials.push_back(matId);
					}
				}
			}

			std::unordered_map<uint32_t, Mesh*> meshesByMaterial;
			for (size_t fi = 0; fi < umesh->num_faces; fi++) {
				const ufbx_face face = umesh->faces.data[fi];

				uint32_t matId = 0;
				if (umesh->face_material.data && fi < umesh->face_material.count) {
					matId = umesh->face_material.data[fi];
				}

				if (cacheForMesh.count(matId))
					continue;

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
					cacheForMesh[matId] = MeshHandle{}; // negative-cache the empty slot
					continue;
				}

				MeshHandle meshHandle = m_renderer->CreateMesh(mesh);
				m_meshes.push_back(meshHandle);
				cacheForMesh[matId] = meshHandle;

				m_meshStats.uniqueMeshes++;
				m_meshStats.totalVertices += mesh->vertexes.size();
				m_meshStats.totalIndices += mesh->indexes.size();
				m_meshStats.totalTriangles += mesh->indexes.size() / 3;
				m_meshStats.totalBytes += mesh->vertexes.size() * sizeof(Vertex) +
				                          mesh->indexes.size() * sizeof(int32_t);

				delete mesh;
			}

			for (uint32_t matId : usedMaterials) {
				auto it = cacheForMesh.find(matId);
				if (it == cacheForMesh.end() || !it->second)
					continue;
				MeshHandle meshHandle = it->second;

				MaterialHandle matHandle{};
				PBRMaterial* material = nullptr;
				if (!m_materialHandles.empty()) {
					size_t safeId = std::min<size_t>(matId, m_materialHandles.size() - 1);
					matHandle = m_materialHandles[safeId];
					material = &m_materials[safeId];
					material->SetHandle(matHandle);
				}

				entt::entity e = m_registry.create();
				m_registry.emplace<TransformComponent>(e, TransformComponent{ worldMatrix });
				m_registry.emplace<MeshComponent>(e, MeshComponent{ meshHandle });
				m_registry.emplace<MaterialComponent>(e, MaterialComponent{ matHandle, material });
			}
		}

		ufbx_free_scene(scene);

		PrintLoadStats();
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

		m_textureStats.uniqueTextures++;
		m_textureStats.fallbackTextures++;
		m_textureStats.totalBytes += fallback.pixels.size();
		m_textureStats.maxWidth = std::max<size_t>(m_textureStats.maxWidth, 1);
		m_textureStats.maxHeight = std::max<size_t>(m_textureStats.maxHeight, 1);

		return fallbackHandle;
	}

	TextureHandle CreateFallbackNormalTexture() {
		static TextureHandle fallbackHandle;
		if (fallbackHandle) {
			return fallbackHandle;
		}

		Image2D fallback;
		fallback.resolution = glm::ivec2(1, 1);
		fallback.format = TextureFormat::RGBA32f;
		fallback.pixels.resize(sizeof(glm::vec4));
		glm::vec4* target = reinterpret_cast<glm::vec4*>(fallback.pixels.data());
		*target = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

		fallbackHandle = m_renderer->CreateTexture(&fallback);

		m_textureStats.uniqueTextures++;
		m_textureStats.fallbackTextures++;
		m_textureStats.totalBytes += fallback.pixels.size();
		m_textureStats.maxWidth = std::max<size_t>(m_textureStats.maxWidth, 1);
		m_textureStats.maxHeight = std::max<size_t>(m_textureStats.maxHeight, 1);

		return fallbackHandle;
	}

	TextureHandle CreateFallbackMetallicTexture() {
		static TextureHandle fallbackHandle;
		if (fallbackHandle) {
			return fallbackHandle;
		}

		Image2D fallback;
		fallback.resolution = glm::ivec2(1, 1);
		fallback.format = TextureFormat::Red32f;
		fallback.pixels.resize(sizeof(float));
		float* target = reinterpret_cast<float*>(fallback.pixels.data());
		*target = 0.0f;

		fallbackHandle = m_renderer->CreateTexture(&fallback);

		m_textureStats.uniqueTextures++;
		m_textureStats.fallbackTextures++;
		m_textureStats.totalBytes += fallback.pixels.size();
		m_textureStats.maxWidth = std::max<size_t>(m_textureStats.maxWidth, 1);
		m_textureStats.maxHeight = std::max<size_t>(m_textureStats.maxHeight, 1);

		return fallbackHandle;
	}

	TextureHandle CreateFallbackRoughnessTexture() {
		static TextureHandle fallbackHandle;
		if (fallbackHandle) {
			return fallbackHandle;
		}

		Image2D fallback;
		fallback.resolution = glm::ivec2(1, 1);
		fallback.format = TextureFormat::Red32f;
		fallback.pixels.resize(sizeof(float));
		float* target = reinterpret_cast<float*>(fallback.pixels.data());
		*target = 1.0f;

		fallbackHandle = m_renderer->CreateTexture(&fallback);

		m_textureStats.uniqueTextures++;
		m_textureStats.fallbackTextures++;
		m_textureStats.totalBytes += fallback.pixels.size();
		m_textureStats.maxWidth = std::max<size_t>(m_textureStats.maxWidth, 1);
		m_textureStats.maxHeight = std::max<size_t>(m_textureStats.maxHeight, 1);

		return fallbackHandle;
	}

	TextureHandle LoadTextureCached(const std::string& path, bool srgb) {
		const std::string key = path + (srgb ? "|srgb" : "|linear");
		if (auto it = m_textureCache.find(key); it != m_textureCache.end()) {
			return it->second;
		}
		TextureHandle h = LoadTexture(path, srgb);
		m_textureCache.emplace(key, h);
		return h;
	}

	TextureHandle LoadTexture(const std::string& filePath, bool srgb) {
		int width = 0, height = 0, channels = 0;
		stbi_uc* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
		if (!data) {
			std::cout << "Failed to load texture: " << filePath << " — using 1x1 white fallback.\n";
			m_textureStats.failedLoads++;
			return srgb ? CreateFallbackTexture() : CreateFallbackNormalTexture();
		}

		std::vector<unsigned char> resized;
		if (width > kMaxTextureSize || height > kMaxTextureSize) {
			const float scale = std::
			    min(static_cast<float>(kMaxTextureSize) / static_cast<float>(width),
			        static_cast<float>(kMaxTextureSize) / static_cast<float>(height));

			const int newW = std::max(1, static_cast<int>(std::lround(width * scale)));
			const int newH = std::max(1, static_cast<int>(std::lround(height * scale)));

			resized.resize(static_cast<size_t>(newW) * newH * 4);

			unsigned char* out = srgb ? stbir_resize_uint8_srgb(
			                                data,
			                                width,
			                                height,
			                                0,
			                                resized.data(),
			                                newW,
			                                newH,
			                                0,
			                                STBIR_RGBA
			                            )
			                          : stbir_resize_uint8_linear(
			                                data,
			                                width,
			                                height,
			                                0,
			                                resized.data(),
			                                newW,
			                                newH,
			                                0,
			                                STBIR_RGBA
			                            );

			if (!out) {
				std::cout << "Resize failed for " << filePath << " — keeping original.\n";
			} else {
				stbi_image_free(data);
				data = resized.data();
				width = newW;
				height = newH;
			}
		}

		Image2D image;
		image.resolution = glm::ivec2(width, height);
		image.format = TextureFormat::RGBA8;
		image.pixels.resize(static_cast<size_t>(width) * height * 4);
		std::memcpy(image.pixels.data(), data, image.pixels.size());

		if (resized.empty()) {
			stbi_image_free(data);
		}

		m_textureStats.uniqueTextures++;
		m_textureStats.loadedFiles++;
		m_textureStats.totalBytes += image.pixels.size();
		m_textureStats.maxWidth = std::max<size_t>(m_textureStats.maxWidth, width);
		m_textureStats.maxHeight = std::max<size_t>(m_textureStats.maxHeight, height);

		return m_renderer->CreateTexture(&image, 13);
	}

	void BeforeDrawFrame() override {
		m_ui->OnBeforeDrawFrame();

		auto& camComp = m_registry.get<CameraComponent>(m_cameraEntity);

		const float dt = PixieApp::Time::deltaTime;
		UpdateFlyCamera(camComp, dt);

		const auto res = m_window->GetResolution();
		const float aspect = static_cast<float>(res.x) / static_cast<float>(res.y);
		camComp.camera.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 5000.0f);

		const glm::vec4 camPos = glm::vec4(glm::vec3(glm::inverse(camComp.camera.view)[3]), 1.0f);

		for (size_t i = 0; i < m_materials.size(); i++) {
			m_materials[i].Bind(m_renderer);

			m_renderer->LoadUniformBuffer(
			    m_materials[i].GetHandle(),
			    "CameraUBO",
			    &camComp.camera,
			    sizeof(Camera)
			);

			m_renderer->LoadUniformBuffer(
			    m_materials[i].GetHandle(),
			    "CameraPosition",
			    &camPos,
			    sizeof(glm::vec4)
			);
		}

		m_meshIslandsMaterial.Bind(m_renderer);
		m_renderer->LoadUniformBuffer(
		    m_meshIslandsMaterialHandle,
		    "CameraUBO",
		    &camComp.camera,
		    sizeof(Camera)
		);

		m_renderer->BeginRenderPass(m_frameBuffer);

		auto view = m_registry.view<TransformComponent, MeshComponent, MaterialComponent>();
		if (m_showmeshIslands) {
			int drawIndex = 0;
			for (auto [entity, transform, meshComp, matComp] : view.each()) {
				struct MeshIslandPushConstants {
					glm::mat4 model;
					glm::vec4 color;
				} pushConstants;
				pushConstants.model = transform.transform;
				pushConstants.color = MeshIslandsMaterial::MakeUniqueDebugColor(drawIndex++);
				m_renderer->DrawMesh(
				    meshComp.mesh,
				    m_meshIslandsMaterialHandle,
				    &pushConstants,
				    sizeof(MeshIslandPushConstants)
				);
			}
		} else {
			for (auto [entity, transform, meshComp, matComp] : view.each()) {
				m_renderer->DrawMesh(
				    meshComp.mesh,
				    matComp.materialHandle,
				    &transform.transform,
				    sizeof(glm::mat4)
				);
			}
		}

		m_renderer->EndRenderPass();
	}

	void UpdateFlyCamera(CameraComponent& cam, float dt) {
		// if (UserInput::IsKeyPressed(GLFW_KEY_TAB)) {
		//	cam.cursorCaptured = !cam.cursorCaptured;
		// }
		// if (UserInput::IsKeyPressed(GLFW_KEY_ESCAPE)) {
		//	cam.cursorCaptured = false;
		// }
		// if (!cam.cursorCaptured && UserInput::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		//	cam.cursorCaptured = true;
		// }
		// UserInput::SetCursorCaptured(cam.cursorCaptured);

		// if (!cam.cursorCaptured) {
		//	return;
		// }

		const bool lookActive = UserInput::IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);

		if (lookActive) {
			const glm::dvec2 md = UserInput::GetMouseDelta();
			cam.yaw += static_cast<float>(md.x) * cam.mouseSensitivity;
			cam.pitch -= static_cast<float>(md.y) * cam.mouseSensitivity;
			cam.pitch = glm::clamp(cam.pitch, -89.0f, 89.0f);
		}

		const float yawRad = glm::radians(cam.yaw);
		const float pitchRad = glm::radians(cam.pitch);
		const glm::vec3 forward(
		    std::cos(pitchRad) * std::cos(yawRad),
		    std::sin(pitchRad),
		    std::cos(pitchRad) * std::sin(yawRad)
		);
		const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
		const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));

		float speed = cam.moveSpeed;
		if (UserInput::IsKeyDown(GLFW_KEY_LEFT_SHIFT)) {
			speed *= 4.0f;
		}

		glm::vec3 vel(0.0f);
		if (UserInput::IsKeyDown(GLFW_KEY_W)) {
			vel += forward;
		}
		if (UserInput::IsKeyDown(GLFW_KEY_S)) {
			vel -= forward;
		}
		if (UserInput::IsKeyDown(GLFW_KEY_D)) {
			vel += right;
		}
		if (UserInput::IsKeyDown(GLFW_KEY_A)) {
			vel -= right;
		}
		if (UserInput::IsKeyDown(GLFW_KEY_SPACE)) {
			vel += worldUp;
		}
		if (UserInput::IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
			vel -= worldUp;
		}

		if (glm::length(vel) > 0.0f) {
			vel = glm::normalize(vel);
			cam.position += vel * speed * dt;
		}

		cam.camera.view = glm::lookAt(cam.position, cam.position + forward, worldUp);
	}

	void PrintLoadStats() const {
		const size_t meshEntities = m_registry.view<MeshComponent>().size();

		const double meshMB = static_cast<double>(m_meshStats.totalBytes) / (1024.0 * 1024.0);
		const double texMB = static_cast<double>(m_textureStats.totalBytes) / (1024.0 * 1024.0);

		std::cout << "\n=== Load statistics ===\n";

		std::cout << "Meshes (unique MeshHandle): " << m_meshStats.uniqueMeshes << '\n';
		std::cout << "Mesh entities: " << meshEntities << '\n';
		std::cout << "Materials: " << m_materials.size() << '\n';
		std::cout << "Vertices: " << m_meshStats.totalVertices << '\n';
		std::cout << "Indices: " << m_meshStats.totalIndices << '\n';
		std::cout << "Triangles: " << m_meshStats.totalTriangles << '\n';
		std::cout << "Mesh data (approx): " << m_meshStats.totalBytes << " bytes (" << meshMB
		          << " MB)\n";

		std::cout << "Textures (unique): " << m_textureStats.uniqueTextures << '\n';
		std::cout << "  loaded files: " << m_textureStats.loadedFiles << '\n';
		std::cout << "  fallback: " << m_textureStats.fallbackTextures << '\n';
		std::cout << "  failed loads: " << m_textureStats.failedLoads << '\n';
		std::cout << "Texture cache entries: " << m_textureCache.size() << '\n';
		std::cout << "Texture data (approx): " << m_textureStats.totalBytes << " bytes (" << texMB
		          << " MB)\n";
		std::cout << "Max texture size: " << m_textureStats.maxWidth << 'x'
		          << m_textureStats.maxHeight << '\n';

		std::cout << "======================\n";
	}

  private:
	FrameBufferHandle m_frameBuffer;
};

int32_t main(void) {
	SponzaSceneApp* app = new SponzaSceneApp(
	    "C:/Repos/PixieRendering/assets/main_sponza/NewSponza_Main_Yup_003.fbx"
	);

	app->Start();

	delete app;

	return 0;
}

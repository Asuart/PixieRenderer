#include <algorithm>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <PixieRendering/Camera/Camera.h>
#include <PixieRendering/Material/PBRMaterial.h>
#include <PixieRendering/PixieRendering.h>
#include <PixieRendering/RenderGraph/IRenderStage.h>
#include <PixieRendering/RenderGraph/RenderGraph.h>
#include <PixieRendering/RenderGraph/RenderGraphBuilder.h>
#include <PixieRendering/RenderGraph/RenderGraphContext.h>
#include <PixieRendering/Renderer/IRenderer.h>
#include <PixieRendering/Renderer/Vulkan/RendererVulkan.h>
#include <PixieRendering/Renderer/Vulkan/VulkanRenderPass.h>
#include <PixieRendering/Window/WindowVulkan.h>

#include <PixieApplication/Time/ApplicationTime.h>
#include <PixieApplication/UserInput/UserInput.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <ufbx.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
// #include <imgui_impl_opengl3.h>   // если понадобится GL

using namespace PixieRenderer;
using namespace PixieApp;

static constexpr int kMaxTextureSize = 2048;
static constexpr glm::uvec2 kRenderSize = { 1280, 720 };

static std::filesystem::path gAssetRoot;

// ============================================================================
// FBX helpers
// ============================================================================

static std::string UfbxStringToStd(const ufbx_string& s) {
	return s.data ? std::string(s.data, s.length) : std::string();
}

static glm::mat4 UfbxMatrixToGlm(const ufbx_matrix& m) {
	glm::mat4 r(1.0f);
	r[0] = glm::vec4(m.cols[0].x, m.cols[0].y, m.cols[0].z, 0.0f);
	r[1] = glm::vec4(m.cols[1].x, m.cols[1].y, m.cols[1].z, 0.0f);
	r[2] = glm::vec4(m.cols[2].x, m.cols[2].y, m.cols[2].z, 0.0f);
	const ufbx_vec3 t = ufbx_transform_position(&m, ufbx_vec3{ 0, 0, 0 });
	r[3] = glm::vec4(t.x, t.y, t.z, 1.0f);
	return r;
}

static std::string ResolveTexturePath(const std::string& filename) {
	if (filename.empty())
		return filename;
	std::filesystem::path p(filename);
	std::error_code ec;
	if (std::filesystem::exists(p, ec))
		return p.string();
	auto local = gAssetRoot / p.filename();
	if (std::filesystem::exists(local, ec))
		return local.string();
	auto rel = gAssetRoot / p;
	if (std::filesystem::exists(rel, ec))
		return rel.string();
	return filename;
}

// ============================================================================
// Scene data
// ============================================================================

struct SceneData {
	struct CameraUBO {
		glm::mat4 view;
		glm::mat4 projection;
	};
	struct CameraState {
		glm::vec3 position = glm::vec3(0.0f, 1.5f, -4.0f);
		float yaw = -90.0f;
		float pitch = -5.0f;
		float moveSpeed = 30.0f;
		float mouseSensitivity = 0.2f;
	} cam;

	CameraUBO cameraData{};

	std::vector<std::unique_ptr<PBRMaterial>> materials;
	std::vector<MaterialHandle> materialHandles;
	std::unordered_map<std::string, TextureHandle> textureCache;
	std::vector<MeshHandle> meshStorage;

	struct DrawItem {
		MeshHandle mesh;
		MaterialHandle material;
		glm::mat4 transform;
	};
	std::vector<DrawItem> items;

	BufferHandle cameraUBO;
	BufferHandle cameraPositionUBO;
};

// ============================================================================
// Texture helpers
// ============================================================================

static TextureHandle CreateSolidTexture(IRenderer* r, glm::vec4 v, TextureFormat fmt) {
	Image2D img;
	img.resolution = { 1, 1 };
	img.format = fmt;

	auto toByte = [](float x) { return static_cast<uint8_t>(std::clamp(x * 255.0f + 0.5f, 0.0f, 255.0f)); };

	switch (fmt) {
	case TextureFormat::RGBA8:
		img.pixels.resize(4);
		img.pixels[0] = toByte(v.x);
		img.pixels[1] = toByte(v.y);
		img.pixels[2] = toByte(v.z);
		img.pixels[3] = toByte(v.w);
		break;
	case TextureFormat::Red8:
		img.pixels.resize(1);
		img.pixels[0] = toByte(v.x);
		break;
	case TextureFormat::RGBA32f:
		img.pixels.resize(sizeof(glm::vec4));
		*reinterpret_cast<glm::vec4*>(img.pixels.data()) = v;
		break;
	case TextureFormat::Red32f:
		img.pixels.resize(sizeof(float));
		*reinterpret_cast<float*>(img.pixels.data()) = v.x;
		break;
	default:
		img.pixels.resize(4);
		img.pixels[0] = toByte(v.x);
		img.pixels[1] = toByte(v.y);
		img.pixels[2] = toByte(v.z);
		img.pixels[3] = toByte(v.w);
		break;
	}
	return r->CreateTexture(&img);
}

static TextureHandle LoadTexture(IRenderer* r, const std::string& path, bool srgb) {
	int w = 0, h = 0, ch = 0;
	stbi_uc* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
	if (!data) {
		std::cout << "Failed to load texture: " << path << "\n";
		return CreateSolidTexture(r, { 1, 1, 1, 1 }, TextureFormat::RGBA32f);
	}

	Image2D img;
	img.format = TextureFormat::RGBA8;

	const bool needResize = (w > kMaxTextureSize || h > kMaxTextureSize);
	if (needResize) {
		const float scale = std::
		    min(static_cast<float>(kMaxTextureSize) / static_cast<float>(w),
		        static_cast<float>(kMaxTextureSize) / static_cast<float>(h));
		const int nw = std::max(1, static_cast<int>(std::lround(w * scale)));
		const int nh = std::max(1, static_cast<int>(std::lround(h * scale)));

		std::vector<stbi_uc> resized(static_cast<size_t>(nw) * nh * 4);
		unsigned char* out = srgb ? stbir_resize_uint8_srgb(data, w, h, 0, resized.data(), nw, nh, 0, STBIR_RGBA)
		                          : stbir_resize_uint8_linear(data, w, h, 0, resized.data(), nw, nh, 0, STBIR_RGBA);
		stbi_image_free(data);

		if (!out) {
			std::cout << "Resize failed: " << path << "\n";
			return CreateSolidTexture(r, { 1, 1, 1, 1 }, TextureFormat::RGBA32f);
		}
		img.resolution = { nw, nh };
		img.pixels.resize(resized.size());
		std::memcpy(img.pixels.data(), resized.data(), resized.size());
	} else {
		img.resolution = { w, h };
		img.pixels.resize(static_cast<size_t>(w) * h * 4);
		std::memcpy(img.pixels.data(), data, img.pixels.size());
		stbi_image_free(data);
	}
	return r->CreateTexture(&img);
}

static TextureHandle LoadTextureCached(SceneData& s, IRenderer* r, const std::string& path, bool srgb) {
	const std::string key = path + (srgb ? "|s" : "|l");
	auto it = s.textureCache.find(key);
	if (it != s.textureCache.end())
		return it->second;
	TextureHandle h = LoadTexture(r, path, srgb);
	s.textureCache.emplace(key, h);
	return h;
}

// ============================================================================
// Scene loading
// ============================================================================

static void LoadScene(SceneData& s, IRenderer* r, const std::string& path) {
	ufbx_error err;
	ufbx_scene* scene = ufbx_load_file(path.c_str(), nullptr, &err);
	if (!scene) {
		std::cerr << "Failed to load FBX\n";
		std::exit(1);
	}
	std::cout << "FBX: materials=" << scene->materials.count << " nodes=" << scene->nodes.count << "\n";

	s.materials.reserve(std::max<size_t>(1, scene->materials.count));
	s.materialHandles.reserve(std::max<size_t>(1, scene->materials.count));

	std::unordered_map<const ufbx_material*, size_t> matIndex;

	if (scene->materials.count == 0) {
		auto mat = std::make_unique<PBRMaterial>();
		mat->SetAlbedo({ 1, 1, 1 });
		mat->SetMetallic(0.0f);
		mat->SetRoughness(0.8f);
		mat->SetAlbedoTexture(CreateSolidTexture(r, { 1, 1, 1, 1 }, TextureFormat::RGBA32f));
		mat->SetNormalTexture(CreateSolidTexture(r, { 0.5f, 0.5f, 1, 1 }, TextureFormat::RGBA32f));
		mat->SetMetallicTexture(CreateSolidTexture(r, { 0, 0, 0, 1 }, TextureFormat::Red32f));
		mat->SetRoughnessTexture(CreateSolidTexture(r, { 1, 1, 1, 1 }, TextureFormat::Red32f));
		MaterialHandle mh = r->CreateMaterial(mat.get());
		mat->SetHandle(mh);
		s.materialHandles.push_back(mh);
		s.materials.push_back(std::move(mat));
		std::cout << "Scene has no materials — using default PBR material.\n";
	} else {
		for (size_t i = 0; i < scene->materials.count; i++) {
			ufbx_material* um = scene->materials.data[i];
			matIndex[um] = i;
			auto mat = std::make_unique<PBRMaterial>();
			const ufbx_vec4 base = um->pbr.base_color.value_vec4;
			mat->SetAlbedo({ base.x, base.y, base.z });
			mat->SetMetallic(static_cast<float>(um->pbr.metalness.value_real));
			mat->SetRoughness(static_cast<float>(um->pbr.roughness.value_real));

			auto resolveTex = [&](const ufbx_texture* tex, glm::vec4 def, TextureFormat fmt, bool srgb
			                  ) -> TextureHandle {
				if (tex && tex->filename.data) {
					std::string p = ResolveTexturePath(UfbxStringToStd(tex->filename));
					return LoadTextureCached(s, r, p, srgb);
				}
				return CreateSolidTexture(r, def, fmt);
			};

			mat->SetAlbedoTexture(resolveTex(um->pbr.base_color.texture, { 1, 1, 1, 1 }, TextureFormat::RGBA32f, true));
			mat->SetNormalTexture(
			    resolveTex(um->pbr.normal_map.texture, { 0.5f, 0.5f, 1, 1 }, TextureFormat::RGBA32f, false)
			);
			mat->SetMetallicTexture(resolveTex(um->pbr.metalness.texture, { 0, 0, 0, 1 }, TextureFormat::Red32f, false)
			);
			mat->SetRoughnessTexture(resolveTex(um->pbr.roughness.texture, { 1, 1, 1, 1 }, TextureFormat::Red32f, false)
			);

			MaterialHandle mh = r->CreateMaterial(mat.get());
			mat->SetHandle(mh);
			s.materialHandles.push_back(mh);
			s.materials.push_back(std::move(mat));
		}
		std::cout << "Loaded " << scene->materials.count << " material(s).\n";
	}

	for (size_t ni = 0; ni < scene->nodes.count; ni++) {
		ufbx_node* node = scene->nodes.data[ni];
		if (!node->mesh || !node->visible)
			continue;
		if (UfbxStringToStd(node->name).contains("decal"))
			continue;

		ufbx_mesh* umesh = node->mesh;
		if (umesh->num_faces == 0 || umesh->num_triangles == 0)
			continue;

		const glm::mat4 world = UfbxMatrixToGlm(node->geometry_to_world);
		std::unordered_map<size_t, Mesh> byMat;

		for (size_t fi = 0; fi < umesh->num_faces; fi++) {
			const ufbx_face face = umesh->faces.data[fi];
			uint32_t faceMatId = (umesh->face_material.data && fi < umesh->face_material.count)
			                         ? umesh->face_material.data[fi]
			                         : 0u;

			ufbx_material* matRef = nullptr;
			if (umesh->materials.data && faceMatId < umesh->materials.count)
				matRef = umesh->materials.data[faceMatId];

			size_t globalMatId = 0;
			auto it = matIndex.find(matRef);
			if (it != matIndex.end())
				globalMatId = it->second;

			Mesh& mesh = byMat[globalMatId];
			for (uint32_t i = 2; i < face.num_indices; i++) {
				const uint32_t tri[3] = { face.index_begin, face.index_begin + i - 1, face.index_begin + i };
				for (int j = 0; j < 3; j++) {
					const uint32_t vi = tri[j];
					Vertex v{};
					auto p = ufbx_get_vertex_vec3(&umesh->vertex_position, vi);
					v.position = { p.x, p.y, p.z };
					if (umesh->vertex_normal.exists) {
						auto n = ufbx_get_vertex_vec3(&umesh->vertex_normal, vi);
						glm::vec3 gn(n.x, n.y, n.z);
						float len = glm::length(gn);
						v.normal = (len > 1e-8f) ? gn / len : glm::vec3(0, 1, 0);
					}
					if (umesh->vertex_uv.exists) {
						auto uv = ufbx_get_vertex_vec2(&umesh->vertex_uv, vi);
						v.uv = { uv.x, uv.y };
					}
					mesh.vertexes.push_back(v);
					mesh.indexes.push_back(static_cast<int32_t>(mesh.vertexes.size() - 1));
				}
			}
		}

		for (auto& [matId, mesh] : byMat) {
			if (mesh.vertexes.empty())
				continue;
			MeshHandle mh = r->CreateMesh(&mesh);
			s.meshStorage.push_back(mh);
			size_t safeId = (matId < s.materialHandles.size()) ? matId : s.materialHandles.size() - 1;
			s.items.push_back({ mh, s.materialHandles[safeId], world });
		}
	}

	ufbx_free_scene(scene);
	std::cout << "Loaded: " << s.items.size() << " draw items\n";
}

// ============================================================================
// ImGui backend (живёт только в main.cpp)
// ============================================================================

class ImGuiBackend {
  public:
	virtual ~ImGuiBackend() = default;
	virtual void Init(GLFWwindow* w, IRenderer* r) = 0;
	virtual void Shutdown() = 0;
	virtual void NewFrame() = 0;
	virtual void Render() = 0;
	virtual void AttachToRenderer(IRenderer* r) = 0;
};

class ImGuiBackendVulkan : public ImGuiBackend {
  public:
	void Init(GLFWwindow* window, IRenderer* r) override {
		m_renderer = dynamic_cast<RendererVulkan*>(r);
		if (!m_renderer)
			throw std::runtime_error("ImGuiBackendVulkan: not a Vulkan renderer");

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(window, true);

		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
		};
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
		poolInfo.poolSizeCount = IM_ARRAYSIZE(poolSizes);
		poolInfo.pPoolSizes = poolSizes;
		if (vkCreateDescriptorPool(m_renderer->GetDevice(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
			throw std::runtime_error("ImGui: descriptor pool failed");

		const uint32_t imgCount = m_renderer->GetSwapchainImageCount();

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.ApiVersion = VK_API_VERSION_1_0;
		init_info.Instance = m_renderer->GetInstance();
		init_info.PhysicalDevice = m_renderer->GetPhysicalDevice();
		init_info.Device = m_renderer->GetDevice();
		init_info.Queue = m_renderer->GetGraphicsQueue();
		init_info.DescriptorPool = m_pool;
		init_info.MinImageCount = imgCount;
		init_info.ImageCount = imgCount;
		init_info.PipelineInfoMain.RenderPass = m_renderer->GetPresentRenderPass();
		init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.PipelineInfoMain.Subpass = 0;

		ImGui_ImplVulkan_Init(&init_info);

		// Хук через публичный IRenderer API — никаких внутренних объектов.
		m_renderer->SetPresentOverlayHook([this]() {
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_renderer->GetCurrentFrameCommandBuffer());
		});
	}

	void Shutdown() override {
		if (m_renderer)
			m_renderer->SetPresentOverlayHook(nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		if (m_pool != VK_NULL_HANDLE && m_renderer) {
			vkDestroyDescriptorPool(m_renderer->GetDevice(), m_pool, nullptr);
			m_pool = VK_NULL_HANDLE;
		}
	}

	void NewFrame() override {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void Render() override {
		ImGui::Render();
		// Ничего не делаем: команды уже записаны через SetAfterDraw
	}

	void AttachToRenderer(IRenderer*) override {
	}

  private:
	RendererVulkan* m_renderer = nullptr;
	VkDescriptorPool m_pool = VK_NULL_HANDLE;
};

// class ImGuiBackendOpenGL : public ImGuiBackend { ... } // аналогично, вызов ImGui_ImplOpenGL3_RenderDrawData
//                                                        // через renderer->SetOverlayDraw(...)

static std::unique_ptr<ImGuiBackend> MakeImGuiBackend(RenderAPI api) {
	switch (api) {
	case RenderAPI::Vulkan:
		return std::make_unique<ImGuiBackendVulkan>();
	// case RenderAPI::OpenGL: return std::make_unique<ImGuiBackendOpenGL>();
	default:
		return nullptr;
	}
}

// ============================================================================
// Present material
// ============================================================================

static const char* cPresentVS = R"(
#version 450
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4 boneWeights;
layout(location = 0) out vec2 vUV;
void main() {
    vUV = aPos.xy * 0.5 + 0.5;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}
)";

static const char* cPresentFS = R"(
#version 450
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 0) uniform sampler2D sceneTexture;
void main() { FragColor = texture(sceneTexture, vUV); }
)";

class PresentMaterial : public IMaterial {
  public:
	PresentMaterial() : IMaterial(cPresentVS, cPresentFS) {
	}
};

// ============================================================================
// Blur compute
// ============================================================================

static const char* cBlurCS = R"(
#version 450
layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba32f) uniform readonly  image2D inputImage;
layout(set = 0, binding = 1, rgba32f) uniform writeonly image2D outputImage;
layout(push_constant) uniform PC { int radius; int _p0, _p1, _p2; } pc;
void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size  = imageSize(outputImage);
    if (coord.x >= size.x || coord.y >= size.y) return;
    vec4 sum = vec4(0.0);
    int count = 0;
    int r = pc.radius;
    for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++) {
        ivec2 c = clamp(coord + ivec2(x, y), ivec2(0), size - 1);
        sum += imageLoad(inputImage, c);
        count++;
    }
    imageStore(outputImage, coord, sum / float(count));
}
)";

class BlurComputeProgram : public IComputeProgram {
  public:
	BlurComputeProgram() : IComputeProgram(cBlurCS) {
	}
};

// ============================================================================
// Stages
// ============================================================================

class SceneStage : public IRenderStage {
  public:
	SceneStage(SceneData& s, RGResource output) : m_scene(s), m_output(output) {
	}
	std::string_view GetName() const override {
		return "Scene";
	}
	StageType GetType() const override {
		return StageType::Graphics;
	}

	void Declare(RenderGraphBuilder& b) override {
		b.Write(m_output, ResourceUsage::ColorAttachment);
		b.SetRenderTarget(m_output);
	}
	void Compile(RenderGraphContext&) override {
	}

	void BindResources(RenderGraphContext& ctx) override {
		IRenderer* r = ctx.GetRenderer();
		const glm::vec4 camPos = glm::vec4(glm::vec3(glm::inverse(m_scene.cameraData.view)[3]), 1.0f);
		r->UpdateBuffer(m_scene.cameraUBO, std::as_bytes(std::span{ &m_scene.cameraData, 1 }));
		r->UpdateBuffer(m_scene.cameraPositionUBO, std::as_bytes(std::span{ &camPos, 1 }));
		for (MaterialHandle mh : m_scene.materialHandles) {
			if (!mh)
				continue;
			r->BindBuffer(mh, "CameraUBO", m_scene.cameraUBO);
			r->BindBuffer(mh, "CameraPosition", m_scene.cameraPositionUBO);
		}
		for (auto& m : m_scene.materials)
			m->Bind(r);
	}

	void Execute(RenderGraphContext& ctx) override {
		IRenderer* r = ctx.GetRenderer();
		for (const auto& item : m_scene.items) {
			DrawRequest req{};
			req.material = item.material;
			req.mesh = item.mesh;
			req.inlineData = std::as_bytes(std::span{ &item.transform, 1 });
			r->DrawMesh(req);
		}
	}

  private:
	SceneData& m_scene;
	RGResource m_output;
};

class BlurStage : public IRenderStage {
  public:
	BlurStage(RGResource in, RGResource out, ComputeProgramHandle prog, glm::uvec2 size)
	    : m_input(in), m_output(out), m_prog(prog), m_size(size) {
	}
	std::string_view GetName() const override {
		return "Blur";
	}
	StageType GetType() const override {
		return StageType::Compute;
	}

	void Declare(RenderGraphBuilder& b) override {
		b.Read(m_input, ResourceUsage::StorageRead, "inputImage");
		b.Write(m_output, ResourceUsage::StorageWrite, "outputImage");
	}
	void Compile(RenderGraphContext&) override {
	}

	void BindResources(RenderGraphContext& ctx) override {
		ctx.BindInputs(m_prog);
	}

	void Execute(RenderGraphContext& ctx) override {
		struct Push {
			int32_t radius;
			int32_t p0, p1, p2;
		} pc{};
		pc.radius = 2;
		DispatchRequest req{};
		req.program = m_prog;
		req.x = (m_size.x + 7) / 8;
		req.y = (m_size.y + 7) / 8;
		req.z = 1;
		req.inlineData = std::as_bytes(std::span{ &pc, 1 });
		ctx.GetRenderer()->DispatchComputeProgram(req);
	}

  private:
	RGResource m_input, m_output;
	ComputeProgramHandle m_prog;
	glm::uvec2 m_size;
};

class PresentStage : public IRenderStage {
  public:
	PresentStage(RGResource input, MeshHandle quad, MaterialHandle mat) : m_input(input), m_quad(quad), m_mat(mat) {
	}
	std::string_view GetName() const override {
		return "Present";
	}
	StageType GetType() const override {
		return StageType::Graphics;
	}

	void Declare(RenderGraphBuilder& b) override {
		m_present = b.ImportPresentTarget("Present");
		b.Read(m_input, ResourceUsage::Sampled, "sceneTexture");
		b.Write(m_present, ResourceUsage::ColorAttachment);
		b.SetRenderTarget(m_present);
	}
	void Compile(RenderGraphContext&) override {
	}

	void BindResources(RenderGraphContext& ctx) override {
		ctx.BindInputs(m_mat);
	}

	void Execute(RenderGraphContext& ctx) override {
		// Ставит в очередь; реальные VkCmd будут записаны в VulkanRenderPass::Execute,
		// в который мы встроили ImGui-хук (см. ImGuiBackendVulkan::Init).
		DrawRequest req{};
		req.material = m_mat;
		req.mesh = m_quad;
		ctx.GetRenderer()->DrawMesh(req);
	}

  private:
	RGResource m_input, m_present;
	MeshHandle m_quad;
	MaterialHandle m_mat;
};

// ============================================================================
// Camera
// ============================================================================

static void UpdateCamera(SceneData& s, float dt, glm::uvec2 resolution) {
	auto& c = s.cam;
	const bool looking = UserInput::IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
	if (looking) {
		auto md = UserInput::GetMouseDelta();
		c.yaw += static_cast<float>(md.x) * c.mouseSensitivity;
		c.pitch -= static_cast<float>(md.y) * c.mouseSensitivity;
		c.pitch = glm::clamp(c.pitch, -89.0f, 89.0f);
	}
	const float yr = glm::radians(c.yaw), pr = glm::radians(c.pitch);
	const glm::vec3 forward(std::cos(pr) * std::cos(yr), std::sin(pr), std::cos(pr) * std::sin(yr));
	const glm::vec3 up(0, 1, 0);
	const glm::vec3 right = glm::normalize(glm::cross(forward, up));

	float speed = c.moveSpeed;
	if (UserInput::IsKeyDown(GLFW_KEY_LEFT_SHIFT))
		speed *= 4.0f;

	glm::vec3 vel(0);
	if (UserInput::IsKeyDown(GLFW_KEY_W))
		vel += forward;
	if (UserInput::IsKeyDown(GLFW_KEY_S))
		vel -= forward;
	if (UserInput::IsKeyDown(GLFW_KEY_D))
		vel += right;
	if (UserInput::IsKeyDown(GLFW_KEY_A))
		vel -= right;
	if (UserInput::IsKeyDown(GLFW_KEY_SPACE))
		vel += up;
	if (UserInput::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
		vel -= up;
	if (glm::length(vel) > 0.0f)
		c.position += glm::normalize(vel) * speed * dt;

	s.cameraData.view = glm::lookAt(c.position, c.position + forward, up);
	const float aspect = float(resolution.x) / float(resolution.y);
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 5000.0f);
	proj[1][1] *= -1.0f; // Vulkan Y-flip
	s.cameraData.projection = proj;
}

// ============================================================================
// main
// ============================================================================

int main() {
	WindowVulkan window("PixieRenderer", glm::ivec2(kRenderSize));
	IRenderer* renderer = window.GetRenderer();

	const std::string scenePath = "C:/Repos/PixieRendering/assets/main_sponza/NewSponza_Main_Yup_003.fbx";
	gAssetRoot = std::filesystem::path(scenePath).parent_path();

	SceneData scene;
	LoadScene(scene, renderer, scenePath);
	scene.cameraUBO = renderer->CreateBuffer(BufferType::Uniform, sizeof(SceneData::CameraUBO));
	scene.cameraPositionUBO = renderer->CreateBuffer(BufferType::Uniform, sizeof(glm::vec4));

	// ---- Present quad ----
	PresentMaterial presentMat;
	MaterialHandle presentMatHandle = renderer->CreateMaterial(&presentMat);

	Mesh fsTri;
	fsTri.vertexes.resize(3);
	fsTri.vertexes[0].position = { -1.0f, -1.0f, 0.0f };
	fsTri.vertexes[1].position = { 3.0f, -1.0f, 0.0f };
	fsTri.vertexes[2].position = { -1.0f, 3.0f, 0.0f };
	fsTri.indexes = { 0, 1, 2 };
	MeshHandle fsTriHandle = renderer->CreateMesh(&fsTri);

	// ---- Blur ----
	BlurComputeProgram blurProgram;
	ComputeProgramHandle blurProgramHandle = renderer->CreateComputeProgram(&blurProgram);

	// ---- Render graph ----
	RenderGraph rg(renderer);

	RenderTargetDesc rtDesc;
	rtDesc.format = TextureFormat::RGBA32f;
	rtDesc.size = kRenderSize;
	rtDesc.hasDepth = true;
	//rtDesc.storageImage = true; // ← нужно для compute
	rtDesc.finalUsage = ResourceUsage::Sampled;
	RGResource sceneColor = rg.RegisterRenderTarget("SceneColor", rtDesc);

	TextureDesc blurDesc;
	blurDesc.format = TextureFormat::RGBA32f;
	blurDesc.size = kRenderSize;
	blurDesc.mipLevels = 1;
	blurDesc.storageImage = true;
	RGResource blurColor = rg.RegisterTexture("BlurColor", blurDesc);

	rg.AddStage(std::make_unique<SceneStage>(scene, sceneColor));
	rg.AddStage(std::make_unique<BlurStage>(sceneColor, blurColor, blurProgramHandle, kRenderSize));
	rg.AddStage(std::make_unique<PresentStage>(blurColor, fsTriHandle, presentMatHandle));
	rg.Compile();

	// ---- ImGui ----
	auto imgui = MakeImGuiBackend(window.GetRenderAPI());
	if (imgui)
		imgui->Init(window.GetGLFWWindow(), renderer);

	// ---- Main loop ----
	while (!window.GetShouldClose()) {
		window.PollEvents();

		if (!renderer->BeginFrame())
			continue;

		if (imgui) {
			imgui->NewFrame();

			ImGui::Begin("Debug");
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::Text("Camera: %.2f %.2f %.2f", scene.cam.position.x, scene.cam.position.y, scene.cam.position.z);
			ImGui::Text("Draw items: %zu", scene.items.size());
			ImGui::End();

			// ImGui::Render() фиксирует draw lists; реальная запись VkCmd
			// произойдёт внутри present render pass через SetAfterDraw hook.
			imgui->Render();
		}

		UpdateCamera(scene, Time::deltaTime, window.GetResolution());
		rg.Execute();
		renderer->EndFrame();
	}

	if (imgui)
		imgui->Shutdown();
	renderer->WaitIdle();
	rg.Clear();
	return 0;
}
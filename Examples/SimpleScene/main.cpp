#include <filesystem>
#include <iostream>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <PixieRendering/PixieRendering.h>
#include <PixieRendering/Resources/Camera.h>

#include <PixieApplication/Time/ApplicationTime.h>
#include <PixieUIApplication/PixieUIApplication.h>
#include <PixieUIApplication/Windows/DemoWindow.h>
#include <PixieUIApplication/Windows/TextureDisplayWindow.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

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

Mesh* LoadMeshOBJ(const std::string& path) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), nullptr);

	if (!warn.empty()) {
		std::cout << "Warning: " << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << "Error: " << err << std::endl;
	}

	if (!ret) {
		std::cerr << "Failed to load/parse .obj file" << std::endl;
		exit(1);
	}

	std::cout << "Loaded " << shapes.size() << " shapes." << std::endl;

	Mesh* mesh = new Mesh();

	for (size_t s = 0; s < shapes.size(); s++) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				Vertex vertex{};

				vertex.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				vertex.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				vertex.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				if (idx.normal_index >= 0) {
					vertex.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
					vertex.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
					vertex.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
				}

				if (idx.texcoord_index >= 0) {
					vertex.uv.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					vertex.uv.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}

				mesh->vertexes.push_back(vertex);
				mesh->indexes.push_back(mesh->indexes.size());
			}
			index_offset += fv;
		}
	}

	return mesh;
}


class SimpleSceneApp : public PixieApp::PixieUIApplication {
  public:
	SimpleSceneApp(const char* path)
	    : PixieUIApplication("Simple scene", { 1280, 720 }, RenderAPI::Vulkan, true) {
		m_frameBuffer = m_renderer->CreateFrameBuffer({ 1280, 720 }, TextureFormat::RGBA32f);

		m_ui->AddWindow(new PixieUI::DemoWindow(m_ui, m_renderer));
		m_ui->AddWindow(new PixieUI::TextureDisplayWindow(m_ui, m_renderer, m_frameBuffer));

		std::filesystem::path appPath = std::filesystem::path(path);
		const std::string filePath = appPath.parent_path().string() + "/cube/cube.obj";

		Mesh* mesh = LoadMeshOBJ(filePath);

		int width, height, channels;
		stbi_uc* data = stbi_load(
		    (appPath.parent_path().string() + "/cube/texture.png").c_str(),
		    &width,
		    &height,
		    &channels,
		    4
		);

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

		m_texture = m_renderer->CreateTexture(&image);

		glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, -5.0f);
		glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
		m_camera.view = glm::lookAt(cameraPosition, center, glm::vec3(0.0f, 1.0f, 0.0f));

		m_meshHandle = m_renderer->CreateMesh(mesh);

		PixieRenderer::IMaterial materialInfo{ "CubeShader", vertexShaderSource, fragmentShaderSource };
		m_materialHandle = m_renderer->CreateMaterial(&materialInfo);

		delete mesh;
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
	MaterialHandle m_materialHandle;
	MeshHandle m_meshHandle;
	TextureHandle m_texture;
	Camera m_camera;
	float m_angle = 0.0f;
};

int32_t main(int argc, char** argv) {
	SimpleSceneApp* app = new SimpleSceneApp(argv[0]);

	app->Start();

	delete app;

	return 0;
}

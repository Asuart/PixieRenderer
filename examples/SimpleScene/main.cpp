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

#include "LoadScene.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

class SimplaeSceneApp : public PixieApp::PixieUIApplication {
  public:
	SimplaeSceneApp(const char* path)
	    : PixieUIApplication("Simple scene", { 1280, 720 }, RenderAPI::Vulkan, true) {
		m_frameBuffer = m_renderer->CreateFrameBuffer({ 1280, 720 }, TextureFormat::RGBA32f);

		m_ui->AddWindow(new PixieUI::DemoWindow(m_ui, m_renderer));
		m_ui->AddWindow(new PixieUI::TextureDisplayWindow(m_ui, m_renderer, m_frameBuffer));

		std::filesystem::path appPath = std::filesystem::path(path);
		const std::string filePath = appPath.parent_path().string() + "/cube/cube.obj";

		Mesh* mesh = LoadMesh(filePath);

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

		Material materialInfo{ vertexShaderSource, fragmentShaderSource };
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
	SimplaeSceneApp* app = new SimplaeSceneApp(argv[0]);

	app->Start();

	delete app;

	return 0;
}

#include <cstring>
#include <iostream>
#include <span>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <PixieRenderer/Buffer/BufferTypes.h>
#include <PixieRenderer/Camera/Camera.h>
#include <PixieRenderer/Image/ImageTypes.h>
#include <PixieRenderer/Material/IMaterial.h>
#include <PixieRenderer/Mesh/Mesh.h>
#include <PixieRenderer/PixieRenderer.h>
#include <PixieRenderer/Renderer/IRenderer.h>
#include <PixieRenderer/Window/IWindow.h>
#include <PixieRenderer/ResourceManager/ResourceHandles.h>

using namespace PixieRenderer;

static const char* vertexShaderSource = R"(
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
    gl_Position = camera.projection * camera.view * modelData.model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* fragmentShaderSource = R"(
#version 450

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;

void main()
{
    int face = int(TexCoord.x + 0.5); // round to nearest integer
    vec3 colors[6] = vec3[](
        vec3(1.0, 0.0, 0.0), // +X red
        vec3(0.0, 1.0, 0.0), // -X green
        vec3(0.0, 0.0, 1.0), // +Y blue
        vec3(1.0, 1.0, 0.0), // -Y yellow
        vec3(1.0, 0.0, 1.0), // +Z magenta
        vec3(0.0, 1.0, 1.0)  // -Z cyan
    );
    FragColor = vec4(colors[face], 1.0);
}
)";

static Mesh* CreateCubeMesh() {
	Mesh* mesh = new Mesh();

	struct V {
		glm::vec3 pos;
		glm::vec3 norm;
		glm::vec2 uv;
	};
	const V verts[] = {
		{ { 1, -1, -1 }, { 1, 0, 0 }, { 0, 0 } },
		{ { 1, 1, -1 }, { 1, 0, 0 }, { 0, 0 } },
		{ { 1, 1, 1 }, { 1, 0, 0 }, { 0, 0 } },
		{ { 1, -1, 1 }, { 1, 0, 0 }, { 0, 0 } },

		{ { -1, -1, 1 }, { -1, 0, 0 }, { 1, 0 } },
		{ { -1, 1, 1 }, { -1, 0, 0 }, { 1, 0 } },
		{ { -1, 1, -1 }, { -1, 0, 0 }, { 1, 0 } },
		{ { -1, -1, -1 }, { -1, 0, 0 }, { 1, 0 } },

		{ { -1, 1, -1 }, { 0, 1, 0 }, { 2, 0 } },
		{ { -1, 1, 1 }, { 0, 1, 0 }, { 2, 0 } },
		{ { 1, 1, 1 }, { 0, 1, 0 }, { 2, 0 } },
		{ { 1, 1, -1 }, { 0, 1, 0 }, { 2, 0 } },

		{ { -1, -1, 1 }, { 0, -1, 0 }, { 3, 0 } },
		{ { -1, -1, -1 }, { 0, -1, 0 }, { 3, 0 } },
		{ { 1, -1, -1 }, { 0, -1, 0 }, { 3, 0 } },
		{ { 1, -1, 1 }, { 0, -1, 0 }, { 3, 0 } },

		{ { -1, -1, 1 }, { 0, 0, 1 }, { 4, 0 } },
		{ { 1, -1, 1 }, { 0, 0, 1 }, { 4, 0 } },
		{ { 1, 1, 1 }, { 0, 0, 1 }, { 4, 0 } },
		{ { -1, 1, 1 }, { 0, 0, 1 }, { 4, 0 } },

		{ { 1, -1, -1 }, { 0, 0, -1 }, { 5, 0 } },
		{ { -1, -1, -1 }, { 0, 0, -1 }, { 5, 0 } },
		{ { -1, 1, -1 }, { 0, 0, -1 }, { 5, 0 } },
		{ { 1, 1, -1 }, { 0, 0, -1 }, { 5, 0 } },
	};

	mesh->vertexes.reserve(std::size(verts));
	for (const auto& v : verts)
		mesh->vertexes.emplace_back(v.pos, v.norm, v.uv);

	const uint32_t indices[] = { 0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
		                         12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20 };
	mesh->indexes.assign(std::begin(indices), std::end(indices));

	return mesh;
}

int main(int /*argc*/, char** /*argv*/) {
	IWindow* window = PixieRenderer::CreateWindow("Rotating Cube", { 1280, 720 }, RenderAPI::Vulkan);

	if (!window) {
		std::cerr << "Failed to create window\n";
		return 1;
	}

	IRenderer* renderer = window->GetRenderer();

	Mesh* cubeMesh = CreateCubeMesh();
	MeshHandle meshHandle = renderer->CreateMesh(cubeMesh);
	delete cubeMesh;

	IMaterial materialInfo(vertexShaderSource, fragmentShaderSource);
	MaterialHandle materialHandle = renderer->CreateMaterial(&materialInfo);

	BufferHandle cameraBuffer = renderer->CreateBuffer(BufferType::Uniform, sizeof(Camera));
	BufferHandle modelBuffer = renderer->CreateBuffer(BufferType::Uniform, sizeof(glm::mat4));

	Camera camera;
	camera.view = glm::lookAt(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	float angle = 0.0f;
	double lastTime = glfwGetTime();

	while (!window->GetShouldClose()) {
		window->PollEvents();

		const double now = glfwGetTime();
		const float deltaTime = static_cast<float>(now - lastTime);
		lastTime = now;

		if (!renderer->BeginFrame())
			continue;

		const glm::ivec2 resolution = window->GetResolution();
		const float aspect = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
		camera.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

		renderer->UpdateBuffer(cameraBuffer, std::as_bytes(std::span{ &camera, 1 }));
		renderer->BindBuffer(materialHandle, "camera", cameraBuffer);

		angle += deltaTime * 0.5f;
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, angle * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, angle * 0.3f, glm::vec3(0.0f, 0.0f, 1.0f));

		renderer->UpdateBuffer(modelBuffer, std::as_bytes(std::span{ &model, 1 }));
		renderer->BindBuffer(materialHandle, "modelData", modelBuffer);

		renderer->BeginRenderPass();
		renderer->DrawMesh({ materialHandle, meshHandle });
		renderer->EndRenderPass();

		renderer->EndFrame();
		window->SwapBuffers();
	}

	delete window;

	return 0;
}

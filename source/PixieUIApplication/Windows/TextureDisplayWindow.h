#pragma once
#include "PixieUIApplication/UIWindow.h"

#include <PixieRendering/Renderer/IRenderer.h>
#include <PixieRendering/ResourceManager/ResourceHandles.h>
#include <PixieUIApplication/UIImage.h>

namespace PixieUI {

class TextureDisplayWindow : public UIWindow {
  public:
	TextureDisplayWindow(UI* ui, PixieRenderer::IRenderer* renderer, PixieRenderer::TextureHandle texture);
	TextureDisplayWindow(UI* ui, PixieRenderer::IRenderer* renderer, PixieRenderer::FrameBufferHandle frameBuffer);
	virtual ~TextureDisplayWindow();

	virtual void OnBeforeDraw() override;
	void Draw() override;

	void SetTexture(PixieRenderer::TextureHandle texture);
	void SetFrameBuffer(PixieRenderer::FrameBufferHandle frameBuffer);

  protected:
	PixieRenderer::FrameBufferHandle m_frameBuffer;
	PixieRenderer::TextureHandle m_targetTexture;
	PixieRenderer::FrameBufferHandle m_targetFrameBuffer;
	PixieRenderer::MaterialHandle m_shader;
	PixieRenderer::MeshHandle m_screenPlane;
	PixieRenderer::BufferHandle m_planeUBO;
	glm::uvec2 m_viewportResolution{};
	glm::uvec2 m_appliedResolution{};
	UIImage* m_image = nullptr;
	bool m_resolutionChanged = true;

	float Aspect(glm::ivec2 resolution);

	void CreateShader();
	void CreateScreenPlane();
	void UpdatePlaneUBO();
};

} // namespace PixieUI

#pragma once
#include "RenderGraphTypes.h"

namespace PixieRenderer {

class RenderGraph;

class RenderGraphBuilder {
  public:
	RenderGraphBuilder(class RenderGraph& graph, int stageIndex);

	// Graph creates the resource via IRenderer, keeps the handle, releases it on Clear.
	RGResource CreateTexture(std::string_view name, const TextureDesc& desc);
	RGResource CreateBuffer(std::string_view name, const BufferDesc& desc);
	RGResource CreateRenderTarget(std::string_view name, const RenderTargetDesc& desc);

	// Externally owned handles. Graph only borrows.
	RGResource ImportTexture(std::string_view name, TextureHandle h);
	RGResource ImportFrameBuffer(std::string_view name, FrameBufferHandle h);
	RGResource ImportPresentTarget(std::string_view name);

	void Read(RGResource r, ResourceUsage usage, std::string_view shaderBinding = {});
	void Write(RGResource r, ResourceUsage usage, std::string_view shaderBinding = {});
	void ReadWrite(RGResource r, ResourceUsage usage, std::string_view shaderBinding = {});

	void SetRenderTarget(RGResource r);
	void SetViewport(ScreenRect rect);
	void SetScissor(ScreenRect rect);

  private:
	RenderGraph& m_graph;
	int m_stageIndex;
};

} // namespace PixieRenderer

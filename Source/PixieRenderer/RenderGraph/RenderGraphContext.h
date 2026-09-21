#pragma once
#include "RenderGraphTypes.h"

namespace PixieRenderer {

class IRenderer;

class RenderGraphContext {
  public:
	RenderGraphContext(class RenderGraph& graph, std::shared_ptr<IRenderer> renderer, int stageIndex);

	std::shared_ptr<IRenderer> GetRenderer() const {
		return m_renderer;
	}

	// Auto-bind every Declare()-input that had a shaderBinding.
	void BindInputs(MaterialHandle material) const;
	void BindInputs(ComputeProgramHandle program) const;

	// Resolve a graph handle to a physical handle.
	TextureHandle GetTexture(RGResource r) const;
	BufferHandle GetBuffer(RGResource r) const;
	FrameBufferHandle GetFrameBuffer(RGResource r) const;

  private:
	friend class RenderGraph;
	RenderGraph& m_graph;
	std::shared_ptr<IRenderer> m_renderer;
	int m_stageIndex;
};

} // namespace PixieRenderer

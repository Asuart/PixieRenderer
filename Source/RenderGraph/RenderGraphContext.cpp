#include "RenderGraphContext.h"
#include "PixieRendering/pch.h"

#include "PixieRendering/Renderer/IRenderer.h"
#include "RenderGraph.h"

namespace PixieRenderer {

RenderGraphContext::RenderGraphContext(RenderGraph& g, IRenderer* r, int i)
    : m_graph(g), m_renderer(r), m_stageIndex(i) {
}

TextureHandle RenderGraphContext::GetTexture(RGResource r) const {
	return m_graph.GetResource(r).texture;
}
BufferHandle RenderGraphContext::GetBuffer(RGResource r) const {
	return m_graph.GetResource(r).buffer;
}
FrameBufferHandle RenderGraphContext::GetFrameBuffer(RGResource r) const {
	return m_graph.GetResource(r).fbo;
}

namespace {

// Один шаблон на оба типа программ — overload resolution подберёт нужный
// IRenderer::BindTexture/BindBuffer.
template <typename ProgramHandle>
void BindOne(IRenderer* r, ProgramHandle p, const RenderGraph::Resource& res, std::string_view name) {
	if (name.empty())
		return;
	if (res.buffer)
		r->BindBuffer(p, name, res.buffer);
	else if (res.fbo)
		r->BindTexture(p, name, res.fbo, 0);
	else if (res.texture)
		r->BindTexture(p, name, res.texture, 0);
}

template <typename ProgramHandle> void BindAll(IRenderer* r, ProgramHandle p, const RenderGraph& g, int stage) {
	const auto& e = g.GetStage(stage);
	for (const auto& in : e.inputs)
		BindOne(r, p, g.GetResource(in.resource), in.shaderBinding);
	for (const auto& out : e.outputs)
		BindOne(r, p, g.GetResource(out.resource), out.shaderBinding);
}

} // namespace

void RenderGraphContext::BindInputs(MaterialHandle m) const {
	BindAll(m_renderer, m, m_graph, m_stageIndex);
}
void RenderGraphContext::BindInputs(ComputeProgramHandle p) const {
	BindAll(m_renderer, p, m_graph, m_stageIndex);
}

} // namespace PixieRenderer
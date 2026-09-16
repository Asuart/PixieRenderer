#include "RenderGraphBuilder.h"
#include "PixieRenderer/pch.h"

#include "RenderGraph.h"

namespace PixieRenderer {

RenderGraphBuilder::RenderGraphBuilder(RenderGraph& g, int i) : m_graph(g), m_stageIndex(i) {
}

RGResource RenderGraphBuilder::CreateTexture(std::string_view n, const TextureDesc& d) {
	return m_graph.RegisterTexture(n, d);
}
RGResource RenderGraphBuilder::CreateBuffer(std::string_view n, const BufferDesc& d) {
	return m_graph.RegisterBuffer(n, d);
}
RGResource RenderGraphBuilder::CreateRenderTarget(std::string_view n, const RenderTargetDesc& d) {
	return m_graph.RegisterRenderTarget(n, d);
}
RGResource RenderGraphBuilder::ImportTexture(std::string_view n, TextureHandle h) {
	return m_graph.ImportTexture(n, h);
}
RGResource RenderGraphBuilder::ImportFrameBuffer(std::string_view n, FrameBufferHandle h) {
	return m_graph.ImportFrameBuffer(n, h);
}
RGResource RenderGraphBuilder::ImportPresentTarget(std::string_view n) {
	return m_graph.ImportPresentTarget(n);
}

void RenderGraphBuilder::Read(RGResource r, ResourceUsage u, std::string_view n) {
	m_graph.RegisterRead(m_stageIndex, r, u, n);
}
void RenderGraphBuilder::Write(RGResource r, ResourceUsage u, std::string_view n) {
	m_graph.RegisterWrite(m_stageIndex, r, u, n);
}
void RenderGraphBuilder::ReadWrite(RGResource r, ResourceUsage u, std::string_view n) {
	m_graph.RegisterReadWrite(m_stageIndex, r, u, n);
}
void RenderGraphBuilder::SetRenderTarget(RGResource r) {
	m_graph.RegisterRenderTarget(m_stageIndex, r);
}
void RenderGraphBuilder::SetViewport(ScreenRect r) {
	m_graph.RegisterViewport(m_stageIndex, r);
}
void RenderGraphBuilder::SetScissor(ScreenRect r) {
	m_graph.RegisterScissor(m_stageIndex, r);
}

} // namespace PixieRenderer

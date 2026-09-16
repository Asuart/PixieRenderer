#include "RenderGraph.h"
#include "PixieRenderer/pch.h"

#include <algorithm>
#include <queue>
#include <stdexcept>

#include "PixieRenderer/Renderer/IRenderer.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphContext.h"
#include "IRenderStage.h"

namespace PixieRenderer {

RenderGraph::RenderGraph(IRenderer* r) : m_renderer(r) {
}

RenderGraph::~RenderGraph() {
	Clear();
}

void RenderGraph::Clear() {
	if (m_renderer) {
		for (auto& r : m_resources) {
			if (!r.graphOwned)
				continue;
			// Ref-counted handle: Reset() drops our ref, ResourceManager decides.
			r.texture.Reset();
			r.buffer.Reset();
			r.fbo.Reset();
		}
	}
	m_resources.clear();
	m_stages.clear();
	m_sortedOrder.clear();
	m_compiled = false;
}

void RenderGraph::AddStage(std::unique_ptr<IRenderStage> stage) {
	StageEntry entry;
	entry.stage = std::move(stage);

	const int index = static_cast<int>(m_stages.size());
	m_stages.push_back(std::move(entry));

	RenderGraphBuilder builder(*this, index);
	m_stages[index].stage->Declare(builder);

	m_compiled = false;
}

// ===========================================================================
// Registration
// ===========================================================================

RGResource RenderGraph::AllocateResource(ResourceKind kind, std::string_view name) {
	RGResource r;
	r.id = static_cast<uint32_t>(m_resources.size());

	Resource res;
	res.name = std::string(name);
	res.kind = kind;

	m_resources.push_back(std::move(res));
	return r;
}

RGResource RenderGraph::RegisterTexture(std::string_view name, const TextureDesc& d) {
	RGResource r = AllocateResource(ResourceKind::Texture, name);
	auto& res = m_resources[r.id];
	res.texDesc = d;

	Image2D empty;
	empty.resolution = glm::ivec2(d.size);
	empty.format = d.format;

	res.texture = m_renderer->CreateTexture(&empty);
	res.graphOwned = true;
	return r;
}

RGResource RenderGraph::RegisterBuffer(std::string_view name, const BufferDesc& d) {
	RGResource r = AllocateResource(ResourceKind::Buffer, name);
	auto& res = m_resources[r.id];
	res.bufDesc = d;
	res.buffer = m_renderer->CreateBuffer(d.type, d.size);
	res.graphOwned = true;
	return r;
}

RGResource RenderGraph::RegisterRenderTarget(std::string_view name, const RenderTargetDesc& d) {
	RGResource r = AllocateResource(ResourceKind::RenderTarget, name);
	auto& res = m_resources[r.id];
	res.rtDesc = d;
	res.fbo = m_renderer->CreateFrameBuffer(d.size, d.format);
	res.graphOwned = true;
	return r;
}

RGResource RenderGraph::ImportTexture(std::string_view name, TextureHandle h) {
	RGResource r = AllocateResource(ResourceKind::ImportedTexture, name);
	m_resources[r.id].texture = h;
	return r;
}
RGResource RenderGraph::ImportFrameBuffer(std::string_view name, FrameBufferHandle h) {
	RGResource r = AllocateResource(ResourceKind::ImportedFrameBuffer, name);
	m_resources[r.id].fbo = h;
	return r;
}
RGResource RenderGraph::ImportPresentTarget(std::string_view name) {
	RGResource r = AllocateResource(ResourceKind::Present, name);
	// Present target — это FrameBufferHandle{}, означающий default framebuffer.
	m_resources[r.id].fbo = FrameBufferHandle{};
	return r;
}

void RenderGraph::RegisterRead(int i, RGResource r, ResourceUsage u, std::string_view n) {
	m_stages[i].inputs.push_back({ r, u, std::string(n) });
	m_compiled = false;
}
void RenderGraph::RegisterWrite(int i, RGResource r, ResourceUsage u, std::string_view n) {
	m_stages[i].outputs.push_back({ r, u, std::string(n) });
	m_compiled = false;
}
void RenderGraph::RegisterReadWrite(int i, RGResource r, ResourceUsage u, std::string_view n) {
	RegisterRead(i, r, u, n);
	RegisterWrite(i, r, u, n);
}
void RenderGraph::RegisterRenderTarget(int i, RGResource r) {
	m_stages[i].targetFBO = r;
}
void RenderGraph::RegisterViewport(int i, ScreenRect v) {
	m_stages[i].hasViewport = true;
	m_stages[i].viewport = v;
}
void RenderGraph::RegisterScissor(int i, ScreenRect s) {
	m_stages[i].hasScissor = true;
	m_stages[i].scissor = s;
}

const RenderGraph::Resource& RenderGraph::GetResource(RGResource r) const {
	if (!r.IsValid() || r.id >= m_resources.size()) {
		throw std::runtime_error("RenderGraph: invalid resource handle");
	}
	return m_resources[r.id];
}
const RenderGraph::StageEntry& RenderGraph::GetStage(int i) const {
	return m_stages[i];
}

// ===========================================================================
// Compile
// ===========================================================================

void RenderGraph::Compile() {
	if (m_stages.empty()) {
		m_compiled = true;
		return;
	}

	TopologicalSort();
	ComputeLifetimes();

	for (int idx : m_sortedOrder) {
		RenderGraphContext ctx(*this, m_renderer, idx);
		m_stages[idx].stage->Compile(ctx);
	}
	m_compiled = true;
}

void RenderGraph::TopologicalSort() {
	const int n = static_cast<int>(m_stages.size());
	std::vector<std::vector<bool>> hasEdge(n, std::vector<bool>(n, false));
	std::vector<int> inDegree(n, 0);

	struct Access {
		int stage;
		bool isWrite;
	};
	std::unordered_map<uint32_t, std::vector<Access>> access;

	for (int i = 0; i < n; ++i) {
		for (const auto& in : m_stages[i].inputs)
			access[in.resource.id].push_back({ i, false });
		for (const auto& out : m_stages[i].outputs)
			access[out.resource.id].push_back({ i, true });
	}

	auto addEdge = [&](int a, int b) {
		if (a == b || hasEdge[a][b])
			return;
		hasEdge[a][b] = true;
		++inDegree[b];
	};

	for (auto& [id, list] : access) {
		for (size_t a = 0; a < list.size(); ++a)
			for (size_t b = a + 1; b < list.size(); ++b) {
				if (!list[a].isWrite && !list[b].isWrite)
					continue;
				addEdge(list[a].stage, list[b].stage);
			}
	}

	std::queue<int> q;
	for (int i = 0; i < n; ++i)
		if (!inDegree[i])
			q.push(i);

	m_sortedOrder.clear();
	while (!q.empty()) {
		int u = q.front();
		q.pop();
		m_sortedOrder.push_back(u);
		for (int v = 0; v < n; ++v)
			if (hasEdge[u][v] && --inDegree[v] == 0)
				q.push(v);
	}

	if (static_cast<int>(m_sortedOrder.size()) != n)
		throw std::runtime_error("RenderGraph: cycle detected");
}

void RenderGraph::ComputeLifetimes() {
	for (auto& r : m_resources) {
		r.firstStage = -1;
		r.lastStage = -1;
	}
	for (size_t p = 0; p < m_sortedOrder.size(); ++p) {
		const int idx = m_sortedOrder[p];
		auto touch = [&](RGResource res) {
			auto& r = m_resources[res.id];
			if (r.firstStage == -1)
				r.firstStage = static_cast<int>(p);
			r.lastStage = static_cast<int>(p);
		};
		for (const auto& in : m_stages[idx].inputs)
			touch(in.resource);
		for (const auto& out : m_stages[idx].outputs)
			touch(out.resource);
	}
}

void RenderGraph::Execute() {
	if (!m_compiled)
		throw std::runtime_error("RenderGraph::Execute before Compile");

	auto applyBarrier = [&](const auto& list) {
		for (const auto& a : list) {
			const auto& r = m_resources[a.resource.id];
			if (r.buffer)
				m_renderer->UseResource(r.buffer, a.usage);
			else if (r.fbo)
				m_renderer->UseResource(r.fbo, a.usage);
			else if (r.texture)
				m_renderer->UseResource(r.texture, a.usage);
		}
	};

for (int idx : m_sortedOrder) {
		StageEntry& e = m_stages[idx];
		const StageType type = e.stage->GetType();

		m_renderer->BeginStage(e.stage->GetName(), type);

		applyBarrier(e.inputs);
		applyBarrier(e.outputs);

		RenderGraphContext ctx(*this, m_renderer, idx);

		e.stage->BindResources(ctx);

		if (type == StageType::Graphics && e.targetFBO.IsValid()) {
			const auto& r = m_resources[e.targetFBO.id];
			m_renderer->SetStageRenderTarget(r.fbo);
		}
		if (e.hasViewport)
			m_renderer->SetStageViewport(e.viewport);
		if (e.hasScissor)
			m_renderer->SetStageScissor(e.scissor);

		e.stage->Execute(ctx);

		m_renderer->EndStage();
	}
}

} // namespace PixieRenderer

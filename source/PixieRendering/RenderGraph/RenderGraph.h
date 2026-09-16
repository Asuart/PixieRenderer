#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "RenderGraphTypes.h"

namespace PixieRenderer {

class IRenderer;
class IRenderStage;

class RenderGraph {
  public:
	explicit RenderGraph(IRenderer* renderer);
	~RenderGraph();

	RenderGraph(const RenderGraph&) = delete;
	RenderGraph& operator=(const RenderGraph&) = delete;

	void Clear();
	void AddStage(std::unique_ptr<IRenderStage> stage);

	void Compile();
	void Execute();

	RGResource RegisterTexture(std::string_view name, const TextureDesc& desc);
	RGResource RegisterBuffer(std::string_view name, const BufferDesc& desc);
	RGResource RegisterRenderTarget(std::string_view name, const RenderTargetDesc& desc);
	RGResource ImportTexture(std::string_view name, TextureHandle h);
	RGResource ImportFrameBuffer(std::string_view name, FrameBufferHandle h);
	RGResource ImportPresentTarget(std::string_view name);

	void RegisterRead(int stageIndex, RGResource r, ResourceUsage u, std::string_view name);
	void RegisterWrite(int stageIndex, RGResource r, ResourceUsage u, std::string_view name);
	void RegisterReadWrite(int stageIndex, RGResource r, ResourceUsage u, std::string_view name);
	void RegisterRenderTarget(int stageIndex, RGResource r);
	void RegisterViewport(int stageIndex, ScreenRect rect);
	void RegisterScissor(int stageIndex, ScreenRect rect);

	friend class RenderGraphBuilder;
	friend class RenderGraphContext;

	struct ResourceInput {
		RGResource resource;
		ResourceUsage usage = ResourceUsage::Sampled;
		std::string shaderBinding;
	};
	struct ResourceOutput {
		RGResource resource;
		ResourceUsage usage = ResourceUsage::ColorAttachment;
		std::string shaderBinding;
	};
	struct Resource {
		std::string name;
		ResourceKind kind = ResourceKind::Texture;

		// Created-by-graph descriptors (for re-creation on resize).
		TextureDesc texDesc;
		BufferDesc bufDesc;
		RenderTargetDesc rtDesc;

		// Physical handles. One of these is set.
		TextureHandle texture;
		BufferHandle buffer;
		FrameBufferHandle fbo;

		// true — graph created it, will Reset() on Clear.
		bool graphOwned = false;

		// Optional lifetime tracking.
		int firstStage = -1;
		int lastStage = -1;
	};
	struct StageEntry {
		std::unique_ptr<IRenderStage> stage;
		std::vector<ResourceInput> inputs;
		std::vector<ResourceOutput> outputs;

		RGResource targetFBO;
		bool hasViewport = false;
		ScreenRect viewport;
		bool hasScissor = false;
		ScreenRect scissor;
	};

  public:
	IRenderer* m_renderer = nullptr;
	std::vector<Resource> m_resources;
	std::vector<StageEntry> m_stages;
	std::vector<int> m_sortedOrder;
	bool m_compiled = false;

	RGResource AllocateResource(ResourceKind kind, std::string_view name);
	void TopologicalSort();
	void ComputeLifetimes();

	const Resource& GetResource(RGResource r) const;
	const StageEntry& GetStage(int stageIndex) const;
};

} // namespace PixieRenderer

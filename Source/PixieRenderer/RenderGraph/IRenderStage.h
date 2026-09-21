#pragma once
#include <memory>
#include <string_view>

#include "RenderGraphTypes.h"

namespace PixieRenderer {

class RenderGraphBuilder;
class RenderGraphContext;
class IRenderer;

class IRenderStage {
  public:
	IRenderStage(std::shared_ptr<IRenderer> renderer) : m_renderer(renderer) {
	}
	virtual ~IRenderStage() = default;

	virtual std::string_view GetName() const = 0;
	virtual StageType GetType() const = 0;

	virtual void Declare(RenderGraphBuilder& builder) = 0;
	virtual void Compile(RenderGraphContext&) = 0;
	virtual void BindResources(RenderGraphContext&) = 0;
	virtual void Execute(RenderGraphContext& ctx) = 0;

  protected:
	std::shared_ptr<IRenderer> m_renderer;
};

} // namespace PixieRenderer

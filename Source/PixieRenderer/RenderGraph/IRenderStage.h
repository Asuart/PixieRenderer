#pragma once
#include <string_view>

#include "RenderGraphTypes.h"

namespace PixieRenderer {

class RenderGraphBuilder;
class RenderGraphContext;

class IRenderStage {
  public:
	virtual ~IRenderStage() = default;

	virtual std::string_view GetName() const = 0;
	virtual StageType GetType() const = 0;

	virtual void Declare(RenderGraphBuilder& builder) = 0;
	virtual void Compile(RenderGraphContext&) = 0;
	virtual void BindResources(RenderGraphContext&) = 0;
	virtual void Execute(RenderGraphContext & ctx) = 0;
};

} // namespace PixieRenderer

#pragma once
#include <PixieApplication/PixieApplication.h>

#include "UI.h"

namespace PixieApp {

class PixieUIApplication : public PixieApplication {
  public:
	PixieUIApplication(
	    const std::string& name,
	    glm::ivec2 resolution,
	    PixieRenderer::RenderAPI renderAPI,
	    bool docking
	);
	virtual ~PixieUIApplication() {
	}

	virtual void BeforeDrawFrame() override;
	void OnDrawFrame() final;
	virtual void OnBeforeDrawUI() {};

  protected:
	PixieUI::UI* m_ui;
};

} // namespace PixieApp

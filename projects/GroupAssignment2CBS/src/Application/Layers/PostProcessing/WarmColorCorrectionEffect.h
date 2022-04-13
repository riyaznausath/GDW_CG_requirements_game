#pragma once
#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture3D.h"

class WarmColorCorrectionEffect : public PostProcessingLayer::Effect {
public:
	MAKE_PTRS(WarmColorCorrectionEffect);
	Texture3D::Sptr Lut;

	WarmColorCorrectionEffect();
	WarmColorCorrectionEffect(bool defaultLut);
	virtual ~WarmColorCorrectionEffect();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	// Inherited from IResource

	WarmColorCorrectionEffect::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _shader;
	float _strength;
};


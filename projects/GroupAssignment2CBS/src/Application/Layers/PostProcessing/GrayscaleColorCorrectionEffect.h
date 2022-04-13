#pragma once
#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture3D.h"

class GrayscaleColorCorrectionEffect : public PostProcessingLayer::Effect {
public:
	MAKE_PTRS(GrayscaleColorCorrectionEffect);
	Texture3D::Sptr Lut;

	GrayscaleColorCorrectionEffect();
	GrayscaleColorCorrectionEffect(bool defaultLut);
	virtual ~GrayscaleColorCorrectionEffect();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	// Inherited from IResource

	GrayscaleColorCorrectionEffect::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _shader;
	float _strength;
};


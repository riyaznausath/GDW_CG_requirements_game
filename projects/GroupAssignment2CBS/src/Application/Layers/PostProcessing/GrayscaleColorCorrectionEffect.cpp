#include "GrayscaleColorCorrectionEffect.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"

GrayscaleColorCorrectionEffect::GrayscaleColorCorrectionEffect() :
	GrayscaleColorCorrectionEffect(true) { }

GrayscaleColorCorrectionEffect::GrayscaleColorCorrectionEffect(bool defaultLut) :
	PostProcessingLayer::Effect(),
	_shader(nullptr),
	_strength(1.0f),
	Lut(nullptr)
{
	Name = "Grayscale Color Correction";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/grayscale_color_correction.glsl" }
	});

	if (defaultLut) {
		Lut = ResourceManager::CreateAsset<Texture3D>("luts/grayscale.CUBE");
	}
	
}

GrayscaleColorCorrectionEffect::~GrayscaleColorCorrectionEffect() = default;

void GrayscaleColorCorrectionEffect::Apply(const Framebuffer::Sptr& gBuffer)
{
	_shader->Bind();
	Lut->Bind(1);
	_shader->SetUniform("u_Strength", _strength);
}

void GrayscaleColorCorrectionEffect::RenderImGui()
{
	LABEL_LEFT(ImGui::LabelText, "LUT", Lut ? Lut->GetDebugName().c_str() : "none");
	LABEL_LEFT(ImGui::SliderFloat, "Strength", &_strength, 0, 1);
}

GrayscaleColorCorrectionEffect::Sptr GrayscaleColorCorrectionEffect::FromJson(const nlohmann::json& data)
{
	GrayscaleColorCorrectionEffect::Sptr result = std::make_shared<GrayscaleColorCorrectionEffect>(false);
	result->Enabled = JsonGet(data, "enabled", true);
	result->_strength = JsonGet(data, "strength", result->_strength);
	result->Lut = ResourceManager::Get<Texture3D>(Guid(data["lut"].get<std::string>()));
	return result;
}

nlohmann::json GrayscaleColorCorrectionEffect::ToJson() const
{
	return {
		{ "enabled", Enabled },
		{ "lut", Lut != nullptr ? Lut->GetGUID().str() : "null" }, 
		{ "strength", _strength }
	};
}

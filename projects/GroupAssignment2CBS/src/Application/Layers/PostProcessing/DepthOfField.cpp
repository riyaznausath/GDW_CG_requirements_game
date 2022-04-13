#include "DepthOfField.h"

#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "../RenderLayer.h"
#include "Application/Application.h"

DepthOfField::DepthOfField() :
	PostProcessingLayer::Effect(),
	_shader(nullptr)
{
	Name = "Depth of Field";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/depthoffield.glsl" }
	});
}

DepthOfField::~DepthOfField() = default;

void DepthOfField::Apply(const Framebuffer::Sptr & gBuffer)
{
	_shader->Bind();
	gBuffer->BindAttachment(RenderTargetAttachment::Depth, 1);
}

void DepthOfField::RenderImGui()
{
	const auto& cam = Application::Get().CurrentScene()->MainCamera;

	if (cam != nullptr) {
		ImGui::DragFloat("Focal Depth", &cam->FocalDepth, 0.1f, 0.1f, 50.0f); //5.0
		ImGui::DragFloat("Lens Dist. ", &cam->LensDepth, 0.01f, 0.001f, 10.f); //0.1
		ImGui::DragFloat("Aperture   ", &cam->Aperture, 0.1f, 0.1f, 7.0f); //7.0
	}
}

DepthOfField::Sptr DepthOfField::FromJson(const nlohmann::json & data)
{
	DepthOfField::Sptr result = std::make_shared<DepthOfField>();
	result->Enabled = JsonGet(data, "enabled", true);
	return result;
}

nlohmann::json DepthOfField::ToJson() const
{
	return {
		{ "enabled", Enabled }
	};
}

#pragma once

#include "../core/texture.hpp"
#include "../core/model.hpp"
#include "../loader/bmp_loader.hpp"
#include "../loader/obj_loader.hpp"
#include "../core/buffer_view.hpp"
#include "../core/software_renderer.hpp"
#include "../framework/fps_renderer_app.hpp"
#include "../framework/resource_manager.hpp"
#include "vertex_type.hpp"
#include "models.hpp"

struct Shader_Unlit
{
	core::Mat mat = core::Mat::Unit();
	core::Texture* tex0 = nullptr;

	Vertex VS(const core::Model_Vertex& v) const
	{
		return Vertex{
		   mat * core::Vec4{v.position,1.0f},
		   core::Vec4(v.normal,1),
		   v.uv,
		   v.normal
		};
	}

	core::Vec4 FS(const Vertex& v) const
	{
		return /*v.color +*/ core::Texture::Sampler(tex0, v.uv);
	}
};

class RenderTest_Unlit final : public framework::FPSRenderAPP
{
private:
	std::shared_ptr<framework::Object> cube;
public:
	RenderTest_Unlit(HINSTANCE hinst) : FPSRenderAPP{ hinst } {}

protected:

	void Init() override
	{
		SoftRasterApp::Init();

		auto tex = loader::bmp::LoadFromFile(L".\\resource\\pictures\\tex0.bmp");
		auto box = loader::obj::LoadFromFile(L".\\resource\\models\\box.obj");
		framework::Resource<core::Model>::Set(L"cube", std::make_shared<core::Model>(std::move(box.value())));
		framework::Resource<core::Texture>::Set(L"tex0", std::make_shared<core::Texture>(std::move(tex.value())));

		camera = std::make_shared<framework::FPSCamera>(core::Vec3{ 0,0,5 }, -90.f);
		cube = world.Spawn<Cube<Shader_Unlit>>();
		//...
	}

	void HandleInput() override
	{
		FPSRenderAPP::HandleInput();

		if (IsKeyPressed<VK_CONTROL, ' '>())
		{
			cube->transform.rotation += core::Vec3{ 1, 1, 1 }*0.01f;
		}
	}
};

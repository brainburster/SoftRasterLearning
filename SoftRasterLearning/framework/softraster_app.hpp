﻿#pragma once

#include <thread>
#include <mutex>
#include <queue>

#include "../core/dc_wnd.hpp"
#include "../core/software_renderer.hpp"
#include "scene.hpp"
#include "render_engine.hpp"

namespace framework
{
	class SoftRasterApp : public IRenderEngine
	{
	protected:
		InputState input_state;
		EngineState engine_state;
		std::queue<MouseMotion, std::list<MouseMotion>> mouse_motions;
		core::DC_WND dc_wnd;
		core::Context<core::Color> ctx;
		core::Context<GbufferType> gbuffer;
		std::shared_ptr<IScene> scene;

		SoftRasterApp(const SoftRasterApp& other) = delete;
		SoftRasterApp& operator=(const SoftRasterApp& other) = delete;

	public:
		SoftRasterApp(HINSTANCE hinst) : dc_wnd{ hinst }, input_state{}, engine_state{}, ctx{}, scene{}
		{
		}

		SoftRasterApp(SoftRasterApp&& other) noexcept :
			input_state{ std::move(other.input_state) },
			engine_state{ std::move(other.engine_state) },
			dc_wnd(std::move(other.dc_wnd)),
			ctx{ std::move(other.ctx) },
			scene{ std::move(other.scene) }
		{
		}

		SoftRasterApp& operator=(SoftRasterApp&& other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}
			memcpy(this, &other, sizeof(SoftRasterApp));
			memset(&other, 0, sizeof(SoftRasterApp));
			return *this;
		}

		virtual const ICamera* GetMainCamera() const override
		{
			return scene->GetMainCamera();
		}

		virtual core::Context<core::Color>& GetCtx() noexcept override
		{
			return ctx;
		}

		virtual core::Context<GbufferType>& GetGBuffer() noexcept override
		{
			return gbuffer;
		}

		virtual const InputState& GetInputState() const noexcept override
		{
			return input_state;
		}

		virtual const EngineState& GetEngineState() const noexcept override
		{
			return engine_state;
		}

		void Run() override
		{
			Init();
			AfterInit();
			HookInput();

			engine_state.time = std::chrono::system_clock::now();;

			std::thread render_theard([&]() {
				while (!dc_wnd.app_should_close())
				{
					UpdateTime();

					HandleInput();
					Update();

					RenderFrame();
					ctx.CopyToBuffer(dc_wnd.GetFrameBufferView());
					dc_wnd.BitBltBuffer();
					EndFrame();
				}
				});

			while (!dc_wnd.app_should_close())
			{
				dc_wnd.PeekMsg();
				//dc_wnd.GetMsg();
				//...
			}

			render_theard.join();
		}

		void UpdateTime()
		{
			auto last = engine_state.time;
			engine_state.time = std::chrono::system_clock::now();
			engine_state.delta = std::chrono::duration_cast<std::chrono::milliseconds>(engine_state.time - last);
			engine_state.delta_count = engine_state.delta.count();
			engine_state.total_time += engine_state.delta_count;
			engine_state.frame_count++;
		}

	protected:
		//为windwos消息添加回调函数，调用一次
		virtual void HookInput() override
		{
			dc_wnd.AddWndProc(WM_KEYDOWN, [&](auto wParam, auto lParam) {
				unsigned char key_id = (unsigned char)LOWORD(wParam);
				input_state.key[key_id] = true;
				return true;
				});

			dc_wnd.AddWndProc(WM_KEYUP, [&](auto wParam, auto lParam) {
				unsigned char key_id = (unsigned char)LOWORD(wParam);
				input_state.key[key_id] = false;
				return true;
				});

			dc_wnd.AddWndProc(WM_MOUSEMOVE, [&](auto wParam, auto lParam) {
				input_state.mouse_state.dx = (short)LOWORD(lParam) - input_state.mouse_state.x;
				input_state.mouse_state.dy = (short)HIWORD(lParam) - input_state.mouse_state.y;
				input_state.mouse_state.x = (short)LOWORD(lParam);
				input_state.mouse_state.y = (short)HIWORD(lParam);

				TRACKMOUSEEVENT track_mouse_event{};
				track_mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
				track_mouse_event.dwFlags = TME_LEAVE;
				track_mouse_event.dwHoverTime = HOVER_DEFAULT;
				track_mouse_event.hwndTrack = dc_wnd.Hwnd();
				TrackMouseEvent(&track_mouse_event);

				//OnMouseMove();
				mouse_motions.push({ WM_MOUSEMOVE,input_state.mouse_state });

				return true;
				});

			dc_wnd.AddWndProc(WM_MOUSELEAVE, [&](auto wParam, auto lParam) {
				input_state.mouse_state = { 0 };
				return true;
				});

			dc_wnd.AddWndProc(WM_LBUTTONDOWN, [&](auto wParam, auto lParam) {
				input_state.mouse_state.button[0] = true;
				return true;
				});

			dc_wnd.AddWndProc(WM_LBUTTONUP, [&](auto wParam, auto lParam) {
				input_state.mouse_state.button[0] = false;
				return true;
				});

			dc_wnd.AddWndProc(WM_RBUTTONDOWN, [&](auto wParam, auto lParam) {
				input_state.mouse_state.button[2] = true;
				return true;
				});

			dc_wnd.AddWndProc(WM_RBUTTONUP, [&](auto wParam, auto lParam) {
				input_state.mouse_state.button[2] = false;
				return true;
				});

			dc_wnd.AddWndProc(WM_MBUTTONDOWN, [&](auto wParam, auto lParam) {
				input_state.mouse_state.button[1] = true;
				return true;
				});

			dc_wnd.AddWndProc(WM_MBUTTONUP, [&](auto wParam, auto lParam) {
				input_state.mouse_state.button[1] = false;
				return true;
				});

			dc_wnd.AddWndProc(WM_MOUSEWHEEL, [&](auto wParam, auto lParam) {
				input_state.mouse_state.scroll = (short)HIWORD(wParam);
				mouse_motions.push({ WM_MOUSEWHEEL,input_state.mouse_state });
				//OnMouseWheel();
				return true;
				});
		}

		//初始化
		virtual void Init() override
		{
			dc_wnd.WndClassName(L"softraster_wnd_cls").WndName(L"空格切换场景").Size(800, 600).RemoveWndStyle(WS_MAXIMIZEBOX).Init();
			ctx.Viewport(800, 600);
			gbuffer.Viewport(800, 600);
		}
		//
		virtual void AfterInit()
		{
			scene->Init(*this);
		}

		//更新
		virtual void Update() override
		{
			scene->Update(*this);
		}

		void TranslateInput()
		{
			for (size_t i = 0; i < 256; ++i)
			{
				input_state.key_pressed[i] = false;
				input_state.key_released[i] = false;
				if (input_state.key[i] != input_state.key_old[i])
				{
					input_state.key_pressed[i] = !input_state.key[i];
					input_state.key_released[i] = input_state.key[i];
				}
				input_state.key_old[i] = input_state.key[i];
			}
			for (size_t i = 0; i < 3; ++i)
			{
				input_state.mouse_state.button_pressed[i] = false;
				input_state.mouse_state.button_released[i] = false;
				if (input_state.mouse_state.button[i] != input_state.mouse_state.button_old[i])
				{
					input_state.mouse_state.button_pressed[i] = !input_state.mouse_state.button[i];
					input_state.mouse_state.button_released[i] = input_state.mouse_state.button[i];
				}
				input_state.mouse_state.button_old[i] = input_state.mouse_state.button[i];
			}
		}

		//每帧调用
		virtual void HandleInput() override
		{
			TranslateInput();
			while (mouse_motions.size())
			{
				auto& motion = mouse_motions.front();
				const_cast<ICamera*>(GetMainCamera())->OnMouseMotion(motion);
				mouse_motions.pop();
			}
			scene->HandleInput(*this);
		}

		//渲染每帧
		virtual void RenderFrame() override
		{
			ctx.Clear({ 0.05f, 0.05f, 0.05f, 1.f });
			scene->RenderFrame(*this);
		}

		//每帧结束前
		virtual void EndFrame() override
		{
		}
	};
}

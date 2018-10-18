#include "hades/gui.hpp"

#include "imgui.h"

#include "SFML/OpenGL.hpp"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/Graphics/VertexArray.hpp"

#include "hades/data.hpp"

namespace hades
{
	gui::gui()
	{
		_activate_context();

		auto &io = ImGui::GetIO();

		// init keyboard mapping
		io.KeyMap[ImGuiKey_Tab] = sf::Keyboard::Tab;
		io.KeyMap[ImGuiKey_LeftArrow] = sf::Keyboard::Left;
		io.KeyMap[ImGuiKey_RightArrow] = sf::Keyboard::Right;
		io.KeyMap[ImGuiKey_UpArrow] = sf::Keyboard::Up;
		io.KeyMap[ImGuiKey_DownArrow] = sf::Keyboard::Down;
		io.KeyMap[ImGuiKey_PageUp] = sf::Keyboard::PageUp;
		io.KeyMap[ImGuiKey_PageDown] = sf::Keyboard::PageDown;
		io.KeyMap[ImGuiKey_Home] = sf::Keyboard::Home;
		io.KeyMap[ImGuiKey_End] = sf::Keyboard::End;
		io.KeyMap[ImGuiKey_Insert] = sf::Keyboard::Insert;
		//see sfml_imgui for android bindings
		io.KeyMap[ImGuiKey_Delete] = sf::Keyboard::Delete;
		io.KeyMap[ImGuiKey_Backspace] = sf::Keyboard::BackSpace;
		io.KeyMap[ImGuiKey_Space] = sf::Keyboard::Space;
		io.KeyMap[ImGuiKey_Enter] = sf::Keyboard::Return;
		io.KeyMap[ImGuiKey_Escape] = sf::Keyboard::Escape;
		io.KeyMap[ImGuiKey_A] = sf::Keyboard::A;
		io.KeyMap[ImGuiKey_C] = sf::Keyboard::C;
		io.KeyMap[ImGuiKey_V] = sf::Keyboard::V;
		io.KeyMap[ImGuiKey_X] = sf::Keyboard::X;
		io.KeyMap[ImGuiKey_Y] = sf::Keyboard::Y;
		io.KeyMap[ImGuiKey_Z] = sf::Keyboard::Z;

		set_display_size({ 1.f, 1.f });

		_generate_atlas();
		
		frame_begin();
		frame_end();
	}

	void gui::activate_context()
	{
		_activate_context();
	}

	void gui::set_display_size(vector_t<float> size)
	{
		_active_assert();
		auto &io = ImGui::GetIO();
		io.DisplaySize = { size.x, size.y };
	}

	const gui::font *gui::add_font(const resources::font *f)
	{
		auto &io = ImGui::GetIO();
		auto &f_atlas = *io.Fonts;
		ImFontConfig cfg;
		cfg.FontDataOwnedByAtlas = false;
		//const cast, because f_atlas demands control of the ptr
		//though it won't actually do anything, since FontDataOwned is set to false
		const auto out = f_atlas.AddFontFromMemoryTTF(const_cast<std::byte*>(f->source_buffer.data()), f->source_buffer.size(), 13.f, &cfg);
		_generate_atlas();

		return out;
	}

	bool gui::handle_event(const sf::Event &e)
	{
		_active_assert();
		auto &io = ImGui::GetIO();
		switch (e.type) {
		case sf::Event::MouseMoved:
			io.MousePos = { static_cast<float>(e.mouseMove.x), 
							static_cast<float>(e.mouseMove.y) };
			return false;
		case sf::Event::MouseButtonPressed:
			io.MouseDown[e.mouseButton.button] = true;
			return io.WantCaptureMouse;
		case sf::Event::MouseButtonReleased:
			io.MouseDown[e.mouseButton.button] = false;
			return io.WantCaptureMouse;
		case sf::Event::MouseWheelMoved:
			io.MouseWheel += static_cast<float>(e.mouseWheel.delta);
			return io.WantCaptureMouse;
		case sf::Event::KeyPressed:
			[[fallthrough]];
		case sf::Event::KeyReleased:
			io.KeysDown[e.key.code] = (e.type == sf::Event::KeyPressed);
			io.KeyCtrl = e.key.control;
			io.KeyShift = e.key.shift;
			io.KeyAlt = e.key.alt;
			return io.WantCaptureKeyboard;
		case sf::Event::TextEntered:
			if (e.text.unicode > 0 && e.text.unicode < 0x10000) {
				io.AddInputCharacter(static_cast<ImWchar>(e.text.unicode));
			}
			return io.WantTextInput;
		default:
			return false;
		}
	}

	void gui::update(time_duration dt)
	{
		//record the change in time
		_activate_context();
		auto &io = ImGui::GetIO();
		io.DeltaTime = time_cast<seconds>(dt).count();
	}

	void gui::frame_begin()
	{
		_active_assert();
		ImGui::NewFrame();
	}

	void gui::frame_end()
	{
		_active_assert();
		ImGui::EndFrame();
	}

	sf::Vertex to_vertex(ImDrawVert vert)
	{
		return { {vert.pos.x, vert.pos.y},
			sf::Color{vert.col},
			{vert.uv.x, vert.uv.y} };
	}

	//draw function borrowed from https://github.com/eliasdaler/imgui-sfml/blob/master/imgui-SFML.cpp
	//proves that my draw function is responsible for the messed up rendering
	void RenderDrawLists(ImDrawData* draw_data)
	{
		if (draw_data->CmdListsCount == 0) {
			return;
		}

		ImGuiIO& io = ImGui::GetIO();
		assert(io.Fonts->TexID != NULL); // You forgot to create and set font texture
		// scale stuff (needed for proper handling of window resize)
		int fb_width = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
		int fb_height = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
		if (fb_width == 0 || fb_height == 0) { return; }
		draw_data->ScaleClipRects(io.DisplayFramebufferScale);

#ifdef GL_VERSION_ES_CL_1_1
		GLint last_program, last_texture, last_array_buffer, last_element_array_buffer;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
#else
		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

#ifdef GL_VERSION_ES_CL_1_1
		glOrthof(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
#else
		glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
#endif

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		for (int n = 0; n < draw_data->CmdListsCount; ++n) {
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->VtxBuffer.front();
			const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();

			glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + offsetof(ImDrawVert, pos)));
			glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + offsetof(ImDrawVert, uv)));
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + offsetof(ImDrawVert, col)));

			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); ++cmd_i) {
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback) {
					pcmd->UserCallback(cmd_list, pcmd);
				}
				else {
					const auto texture = static_cast<const resources::texture*>(pcmd->TextureId);
					glBindTexture(GL_TEXTURE_2D, texture->value.getNativeHandle());
					glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w),
						(int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
					glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer);
				}
				idx_buffer += pcmd->ElemCount;
			}
		}
#ifdef GL_VERSION_ES_CL_1_1
		glBindTexture(GL_TEXTURE_2D, last_texture);
		glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
		glDisable(GL_SCISSOR_TEST);
#else
		glPopAttrib();
#endif
	}

	//TODO: find out why this isn't working
	//because we don't have glscissor?
	void gui::draw(sf::RenderTarget & target, sf::RenderStates states) const
	{
		_active_assert();

		ImGui::Render();
		const auto draw_data = ImGui::GetDrawData();
		assert(draw_data);

		//let someone elses better render function do it for us
		//NOTE: this doesn't respect sf::view settings
		RenderDrawLists(draw_data);
		return;

		const auto first = draw_data->CmdLists;
		const auto last = draw_data->CmdLists + draw_data->CmdListsCount;

		//for each entry in the draw list
		std::for_each(first, last, [&target, states](ImDrawList *draw_list) {
			const auto *index_first = draw_list->IdxBuffer.Data;

			//for each command
			for (const auto cmd : draw_list->CmdBuffer)
			{
				if (cmd.UserCallback)
					std::invoke(cmd.UserCallback, draw_list, &cmd);
				else
				{
					//get the verts from the draw list that are associated with
					//this command
					sf::VertexArray verts{ sf::Triangles, cmd.ElemCount };
					const auto *i = index_first;
					for (decltype(ImDrawCmd::ElemCount) j = 0; j < cmd.ElemCount; ++j)
						verts[j] = to_vertex(draw_list->VtxBuffer[*(i++)]);

					if (cmd.TextureId)
					{
						const auto texture = static_cast<const resources::texture*>(cmd.TextureId);
						assert(texture);
						auto state = states;
						state.texture = &texture->value;
						target.draw(verts, state);
					}
					else
						target.draw(verts);
				}

				//move the index ptr forward for the next command
				index_first += cmd.ElemCount;
			}
		});
	}

	void gui::show_demo_window()
	{
		_active_assert();
		ImGui::ShowDemoWindow();
	}

	constexpr std::string_view gui::version()
	{
		return IMGUI_VERSION;
	}

	void gui::_activate_context()
	{
		assert(_my_context);

		ImGui::SetCurrentContext(_my_context.get());
	}

	void gui::_active_assert() const
	{
		assert(_my_context);
		assert(ImGui::GetCurrentContext() == _my_context.get());
	}

	void gui::_generate_atlas()
	{
		//get texture
		auto [d, lock] = data::detail::get_data_manager_exclusive_lock();
		std::ignore = lock;

		auto t = d->find_or_create<resources::texture>(_font_atlas_id, unique_id::zero);

		t->mips = false;
		t->repeat = false;
		t->smooth = false;

		//get the data and set the correct ids
		auto &io = ImGui::GetIO();
		auto &f_atlas = *io.Fonts;
		int width = 0, height = 0;
		unsigned char *texture_data = nullptr;
		f_atlas.GetTexDataAsRGBA32(&texture_data, &width, &height);
		f_atlas.TexID = static_cast<void*>(t);

		//make the texture
		t->value.create(width, height);
		t->value.update(texture_data);
		//apply correct settings
		t->value.setRepeated(false);
		t->value.setSmooth(false);
		t->width = width;
		t->height = height;
	}
}
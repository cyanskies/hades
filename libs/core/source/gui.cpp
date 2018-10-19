#include "hades/gui.hpp"

#include "imgui.h"

#include "SFML/OpenGL.hpp"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/Graphics/VertexArray.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"
#include "hades/font.hpp"
#include "hades/texture.hpp"

namespace hades
{
	gui::gui()
	{
		if (!_font_atlas)
			_font_atlas = std::make_unique<ImFontAtlas>();

		_my_context = context_ptr{ ImGui::CreateContext(_font_atlas.get()), ImGui::DestroyContext };
		_activate_context(); //context will only be activated by default if it is the only one

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

	void gui::show_demo_window()
	{
		_active_assert();
		ImGui::ShowDemoWindow();
	}

	sf::Vertex to_vertex(ImDrawVert vert, sf::Vector2f tex_size = { 1.f, 1.f })
	{
		const auto col = ImColor{ vert.col }.Value;
		
		return { {vert.pos.x, vert.pos.y},
			sf::Color{static_cast<sf::Uint8>(col.x * 255.f),
					static_cast<sf::Uint8>(col.y * 255.f), 
					static_cast<sf::Uint8>(col.z * 255.f), 
					static_cast<sf::Uint8>(col.w * 255.f)},
			//uv coords are normalised for the texture size as [0.f, 1.f]
			//we need to expand them to the range of [0, tex_size]
			//same as the colours above
			{vert.uv.x * tex_size.x, vert.uv.y * tex_size.y} };
	}

	bool gui::window_begin(std::string_view name, bool &closed, window_flags flags)
	{
		_active_assert();
		return ImGui::Begin(to_string(name).data(), &closed, static_cast<ImGuiWindowFlags>(flags));
	}

	bool gui::window_begin(std::string_view name, window_flags flags)
	{
		_active_assert();
		return ImGui::Begin(to_string(name).data(), nullptr, static_cast<ImGuiWindowFlags>(flags));;
	}

	void gui::window_end()
	{
		_active_assert();
		ImGui::End();
	}

	bool gui::child_window_begin(std::string_view name, vector size, bool border, window_flags flags)
	{
		_active_assert();
		return ImGui::BeginChild(to_string(name).data(), { size.x, size.y }, border, static_cast<ImGuiWindowFlags>(flags));
	}

	void gui::child_window_end()
	{
		_active_assert();
		ImGui::EndChild();
	}

	gui::vector gui::window_position() const
	{
		_active_assert();
		const auto pos = ImGui::GetWindowPos();
		return { pos.x, pos.y };
	}

	gui::vector gui::window_size() const
	{
		_active_assert();
		const auto size = ImGui::GetWindowSize();
		return { size.x, size.y };
	}

	void gui::push_font(const resources::font *f)
	{
		_active_assert();
		auto font = _get_font(f);
		ImGui::PushFont(font);
	}

	void gui::pop_font()
	{
		_active_assert();
		ImGui::PopFont();
	}

	ImVec4 to_imvec4(const sf::Color &c)
	{
		return {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
	}

	void gui::image(const resources::animation &a, const vector &size, time_point time, const sf::Color &tint_colour, const sf::Color &border_colour)
	{
		_active_assert();
		const auto[x, y] = animation::get_frame(a, time);

		const auto tex_width = a.tex->width;
		const auto tex_height = a.tex->height;

		ImGui::Image(const_cast<resources::texture*>(a.tex), //ImGui only accepts these as non-const void* 
			{ size.x, size.y },
			{ x / tex_width, y / tex_height }, // normalised coords
			{ (x + a.width) / tex_width,  (y + a.height) / tex_width }, // absolute pos for bottom right corner, also normalised
			to_imvec4(tint_colour),
			to_imvec4(border_colour));
	}

	bool gui::image_button(const resources::animation &a, const vector &size, time_point time, const sf::Color & background_colour, const sf::Color & tint_colour)
	{
		_active_assert();
		const auto[x, y] = animation::get_frame(a, time);

		const auto tex_width = a.tex->width;
		const auto tex_height = a.tex->height;

		return ImGui::ImageButton(const_cast<resources::texture*>(a.tex), //ImGui only accepts these as non-const void* 
			{ size.x, size.y },
			{ x / tex_width, y / tex_height }, // normalised coords
			{ (x + a.width) / tex_width,  (y + a.height) / tex_width }, // absolute pos for bottom right corner, also normalised
			-1, // frame padding
			to_imvec4(background_colour),
			to_imvec4(tint_colour));
	}

	//NOTE:mixing gl commands in order to get clip clipping scissor glscissor
	void gui::draw(sf::RenderTarget & target, sf::RenderStates states) const
	{
		_active_assert();

		ImGui::Render();
		const auto draw_data = ImGui::GetDrawData();
		assert(draw_data);

		const auto first = draw_data->CmdLists;
		const auto last = draw_data->CmdLists + draw_data->CmdListsCount;

		const auto view = target.getView();
		const auto view_height = view.getSize().y;

		//for each entry in the draw list
		std::for_each(first, last, [&target, states, view_height](ImDrawList *draw_list) {
			const auto *index_first = draw_list->IdxBuffer.Data;

			//for each command
			for (const auto cmd : draw_list->CmdBuffer)
			{
				if (cmd.UserCallback)
					std::invoke(cmd.UserCallback, draw_list, &cmd);
				else
				{
					//get the info needed to denormalise the tex coords.
					sf::Vector2f texture_size = { 1.f, 1.f };
					if (cmd.TextureId)
					{
						const auto texture = static_cast<const resources::texture*>(cmd.TextureId);
						texture_size = { static_cast<float>(texture->width), static_cast<float>(texture->height) };
					}

					//get the verts from the draw list that are associated with
					//this command
					sf::VertexArray verts{ sf::Triangles, cmd.ElemCount };
					const auto *i = index_first;
					for (decltype(ImDrawCmd::ElemCount) j = 0; j < cmd.ElemCount; ++j)
						verts[j] = to_vertex(draw_list->VtxBuffer[*(i++)], texture_size);


					glEnable(GL_SCISSOR_TEST);
					glScissor(static_cast<GLsizei>(cmd.ClipRect.x),
						static_cast<GLsizei>(view_height - cmd.ClipRect.w),
						static_cast<GLsizei>(cmd.ClipRect.z - cmd.ClipRect.x),
						static_cast<GLsizei>(cmd.ClipRect.w - cmd.ClipRect.y));
					//draw with texture
					if (cmd.TextureId)
					{
						const auto texture = static_cast<const resources::texture*>(cmd.TextureId);
						assert(texture);
						auto state = states;
						state.texture = &texture->value;
						target.draw(verts, state);
					}
					else //draw coloured verts
						target.draw(verts);

					glDisable(GL_SCISSOR_TEST);
				}

				//move the index ptr forward for the next command
				index_first += cmd.ElemCount;
			}
		});
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

	gui::font *gui::_get_font(const resources::font *f)
	{
		assert(f);

		if (const auto font = _fonts.find(f); font != std::end(_fonts))
			return font->second;
		else
			return _create_font(f);
	}

	//const gui::font *gui::add_font(const resources::font *f)
	//{
	//	auto &io = ImGui::GetIO();
	//	auto &f_atlas = *io.Fonts;
	//	ImFontConfig cfg;
	//	cfg.FontDataOwnedByAtlas = false;
	//	//const cast, because f_atlas demands control of the ptr
	//	//though it won't actually do anything, since FontDataOwned is set to false
	//	const auto out = f_atlas.AddFontFromMemoryTTF(const_cast<std::byte*>(f->source_buffer.data()), f->source_buffer.size(), 13.f, &cfg);
	//	_generate_atlas();

	//	return out;
	//}

	gui::font *gui::_create_font(const resources::font *f)
	{
		auto &io = ImGui::GetIO();
		auto &f_atlas = *_font_atlas;
		ImFontConfig cfg;
		cfg.FontDataOwnedByAtlas = false;
		//const cast, because f_atlas demands control of the ptr
		//though it won't actually do anything, since FontDataOwned is set to false
		const auto out = f_atlas.AddFontFromMemoryTTF(const_cast<std::byte*>(f->source_buffer.data()), f->source_buffer.size(), 13.f, &cfg);
		_generate_atlas();

		return out;
	}

	void gui::_generate_atlas()
	{
		//get texture
		auto [d, lock] = data::detail::get_data_manager_exclusive_lock();
		std::ignore = lock;

		static unique_id font_texture_id{};

		auto t = d->find_or_create<resources::texture>(font_texture_id, unique_id::zero);

		t->mips = false;
		t->repeat = false;
		t->smooth = false;

		//get the data and set the correct ids
		auto &io = ImGui::GetIO();
		auto &f_atlas = *_font_atlas;
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

	//gui::static objects
	std::unique_ptr<ImFontAtlas> gui::_font_atlas{ nullptr };
	std::unordered_map<const resources::font*, gui::font*> gui::_fonts;
}
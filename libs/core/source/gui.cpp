#include "hades/gui.hpp"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "SFML/OpenGL.hpp"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/Graphics/VertexArray.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"
#include "hades/draw_clamp.hpp"
#include "hades/font.hpp"
#include "hades/texture.hpp"

namespace hades
{
	gui::gui()
	{
		if (!_font_atlas)
			_font_atlas = std::make_unique<ImFontAtlas>();
		assert(_font_atlas);

		_my_context = context_ptr{ ImGui::CreateContext(_font_atlas.get()) };
		assert(_my_context);

		const auto prev_context = ImGui::GetCurrentContext();

		_activate_context(); //context will only be activated by default if it is the only one

		auto &io = ImGui::GetIO();

		io.BackendPlatformName = "hades"; //TODO: this should be game_name()
		io.BackendRendererName = "hades SFML";

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

		ImGui::SetCurrentContext(prev_context);
	}

	void gui::activate_context() noexcept
	{
		_activate_context();
	}

	void gui::set_display_size(vector_t<float> size)
	{
		_active_assert();
		auto &io = ImGui::GetIO();
		io.DisplaySize = { size.x, size.y };

		_main_toolbar_info.width = size.x;
	}

	bool gui::handle_event(const sf::Event &e)
	{
		_active_assert();
		auto &io = ImGui::GetIO();
		switch (e.type) {
		case sf::Event::GainedFocus:
			io.AddFocusEvent(true);
			return false;
		case sf::Event::LostFocus:
			io.AddFocusEvent(false);
			return false;
		case sf::Event::MouseMoved:
			io.MousePos = { static_cast<float>(e.mouseMove.x), 
							static_cast<float>(e.mouseMove.y) };
			return false; // TODO: maybe return io.WantCaptureMouse;
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
				const auto str = sf::String{ e.text.unicode };
				const auto utf8 = str.toUtf8();
				auto std_str = string{};
				std::transform(begin(utf8), end(utf8), std::back_inserter(std_str), [](sf::Uint8 u8)noexcept->char{
					auto output = char{};
					std::memcpy(&output, &u8, sizeof(char));
					return output;
				});
				io.AddInputCharactersUTF8(std_str.data());
			}
			return io.WantTextInput;
		default:
			return false;
		}
	}

	void gui::update(time_duration dt)
	{
		_active_assert();
		//record the change in time
		auto &io = ImGui::GetIO();
		io.DeltaTime = seconds_float{ dt }.count();
	}

	void gui::frame_begin()
	{
		_active_assert();

		_main_toolbar_info.last_item_x2 = 0.f;
		_main_toolbar_info.main_menubar_y2 = 0.f;

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

	bool gui::child_window_begin(std::string_view name, vector2 size, bool border, window_flags flags)
	{
		_active_assert();
		return ImGui::BeginChild(to_string(name).data(), { size.x, size.y }, border, static_cast<ImGuiWindowFlags>(flags));
	}

	void gui::child_window_end()
	{
		_active_assert();
		ImGui::EndChild();
	}

	bool gui::is_window_appearing() const
	{
		_active_assert();
		return ImGui::IsWindowAppearing();
	}

	gui::vector2 gui::window_position() const
	{
		_active_assert();
		const auto pos = ImGui::GetWindowPos();
		return { pos.x, pos.y };
	}

	gui::vector2 gui::window_size() const
	{
		_active_assert();
		const auto size = ImGui::GetWindowSize();
		return { size.x, size.y };
	}

	void gui::next_window_position(vector2 p)
	{
		_active_assert();
		ImGui::SetNextWindowPos({ p.x, p.y });
	}

	void gui::next_window_size(vector2 s)
	{
		_active_assert();
		ImGui::SetNextWindowSize({ s.x, s.y });
	}

	float gui::get_scroll_x() const
	{
		_active_assert();
		return ImGui::GetScrollX();
	}

	float gui::get_scroll_y() const
	{
		_active_assert();
		return ImGui::GetScrollY();
	}

	void gui::set_scroll_x(float scroll_x)
	{
		_active_assert();
		ImGui::SetScrollX(scroll_x);
		return;
	}

	void gui::set_scroll_y(float scroll_y)
	{
		_active_assert();
		ImGui::SetScrollY(scroll_y);
	}

	float gui::get_scroll_max_x() const
	{
		_active_assert();
		return ImGui::GetScrollMaxX();
	}

	float gui::get_scroll_max_y() const
	{
		_active_assert();
		return ImGui::GetScrollMaxY();
	}

	void gui::set_scroll_here_x(float f)
	{
		_active_assert();
		ImGui::SetScrollHereX(f);
		return;
	}

	void gui::set_scroll_here_y(float f)
	{
		_active_assert();
		ImGui::SetScrollHereY(f);
		return;
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
		return { c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f };
	}

	void gui::push_colour(colour_target element, const sf::Color &c)
	{
		_active_assert();
		ImGui::PushStyleColor(static_cast<ImGuiCol>(element), to_imvec4(c));
	}

	void gui::pop_colour()
	{
		_active_assert();
		ImGui::PopStyleColor();
	}

	void gui::push_item_width(float width)
	{
		_active_assert();
		ImGui::PushItemWidth(width);
	}

	void gui::pop_item_width()
	{
		_active_assert();
		ImGui::PopItemWidth();
	}

	void gui::push_text_wrap_pos(float f)
	{
		_active_assert();
		ImGui::PushTextWrapPos(f);
		return;
	}

	void gui::pop_text_wrap_pos()
	{
		_active_assert();
		ImGui::PopTextWrapPos();
		return;
	}

	void gui::separator_horizontal()
	{
		_active_assert();
		ImGui::Separator();
	}

	void gui::layout_horizontal(float pos, float width)
	{
		_active_assert();
		ImGui::SameLine(pos, width);
	}

	void gui::layout_vertical()
	{
		_active_assert();
		ImGui::NewLine();
	}

	void gui::vertical_spacing()
	{
		_active_assert();
		ImGui::Spacing();
	}

	void gui::group_begin()
	{
		_active_assert();
		ImGui::BeginGroup();
	}

	void gui::group_end()
	{
		_active_assert();
		ImGui::EndGroup();
	}

	void gui::dummy(vector_float s)
	{
		_active_assert();
		ImGui::Dummy({ s.x, s.y });
	}

	float gui::get_frame_height_with_spacing() const noexcept
	{
		_active_assert();
		return ImGui::GetFrameHeightWithSpacing();
	}

	void gui::push_id(std::string_view s)
	{
		_active_assert();
		assert(!std::empty(s));
		auto *last = &*(--std::end(s));
		ImGui::PushID(&*std::begin(s), ++last);
	}

	void gui::push_id(int32 i)
	{
		_active_assert();
		ImGui::PushID(i);
	}

	void gui::push_id(const void *ptr)
	{
		_active_assert();
		ImGui::PushID(ptr);
	}

	void gui::pop_id()
	{
		_active_assert();
		ImGui::PopID();
	}

	void gui::text(std::string_view s)
	{
		_active_assert();
		ImGui::TextUnformatted(s.data(), s.data() + s.size());
	}

	void gui::text_coloured(std::string_view s, const sf::Color &c)
	{
		_active_assert();
		push_colour(colour_target::text, c);
		text(s);
		pop_colour();
	}

	void gui::text_disabled(std::string_view s)
	{
		_active_assert();
		const auto &style = ImGui::GetStyle();
		const auto &c = style.Colors[static_cast<ImGuiCol>(colour_target::text_disabled)];
		ImGui::PushStyleColor(static_cast<ImGuiCol>(colour_target::text), c);
		text(s);
		pop_colour();	
	}

	void gui::text_wrapped(std::string_view s)
	{
		_active_assert();
		ImGui::TextWrapped(to_string(s).data());
		return;
	}

	void gui::text_bullet(std::string_view s)
	{
		_active_assert();
		bullet();
		text(s);
	}

	bool gui::button(std::string_view label, const vector2 &size)
	{
		_active_assert();
		return ImGui::Button(to_string(label).data(), { size.x, size.y });
	}

	bool gui::small_button(std::string_view label)
	{
		_active_assert();
		return ImGui::SmallButton(to_string(label).data());
	}

	bool gui::invisible_button(std::string_view label, const vector2 & size)
	{
		_active_assert();
		return ImGui::InvisibleButton(to_string(label).data(), { size.x, size.y });
	}

	bool gui::arrow_button(std::string_view label, direction d)
	{
		_active_assert();
		return ImGui::ArrowButton(to_string(label).data(), static_cast<ImGuiDir>(d));
	}

	void gui::image(const resources::texture &t, const rect_float &text_coords, const vector2 &size, const sf::Color &tint_colour, const sf::Color &border_colour)
	{
		_active_assert();

		const auto x = text_coords.x,
			y = text_coords.y,
			width = text_coords.width,
			height = text_coords.height;

		namespace tex = resources::texture_functions;
		const auto [tex_width, tex_height] = tex::get_size(&t);

		ImGui::Image(const_cast<resources::texture*>(&t), //ImGui only accepts these as non-const void* 
			{ size.x, size.y },
			{ x / tex_width, y / tex_height }, // normalised coords
			{ (x + width) / tex_width,  (y + height) / tex_width }, // absolute pos for bottom right corner, also normalised
			to_imvec4(tint_colour),
			to_imvec4(border_colour));
	}

	void gui::image(const resources::animation &a, const vector2 &size, time_point time, const sf::Color &tint_colour, const sf::Color &border_colour)
	{
		_active_assert();
		const auto& f = animation::get_frame(a, time);
		const auto texture = resources::animation_functions::get_texture(a);
		assert(texture);
		return image(*texture,
			rect_float{ f.x, f.y, f.w, f.h },
			size, 
			tint_colour,
			border_colour
		);
	}

	bool gui::image_button(const resources::texture &texture, const rect_float &text_coords, const vector2 &size, const sf::Color &background_colour, const sf::Color &tint_colour)
	{
		_active_assert();

		const auto x = text_coords.x,
			y = text_coords.y,
			width = text_coords.width,
			height = text_coords.height;

		namespace tex = resources::texture_functions;
		const auto [tex_width, tex_height] = tex::get_size(&texture);

		return ImGui::ImageButton(const_cast<resources::texture*>(&texture), //ImGui only accepts these as non-const void* 
			{ size.x, size.y },
			{ x / tex_width, y / tex_height }, // normalised coords
			{ (x + width) / tex_width,  (y + height) / tex_height }, // absolute pos for bottom right corner, also normalised
			-1, // frame padding
			to_imvec4(background_colour),
			to_imvec4(tint_colour));
	}

	bool gui::image_button(const resources::animation &a, const vector2 &size, time_point time, const sf::Color & background_colour, const sf::Color & tint_colour)
	{
		_active_assert();
		const auto& f = animation::get_frame(a, time);
		const auto texture = resources::animation_functions::get_texture(a);
		assert(texture);

		//push the animation address, so that animations with the same texture
		// dont have the same id
		push_id(&a);
		const auto result = image_button(
			*texture,
			rect_float{ f.x, f.y, f.w, f.h },
			size,
			background_colour,
			tint_colour
		);
		pop_id();

		return result;
	}

	bool gui::checkbox(std::string_view label, bool &checked)
	{
		_active_assert();
		return ImGui::Checkbox(to_string(label).data(), &checked);
	}

	bool gui::radio_button(std::string_view label, bool active)
	{
		_active_assert();
		return ImGui::RadioButton(to_string(label).data(), active);
	}

	void gui::progress_bar(float progress, const vector2 &size)
	{
		_active_assert();
		ImGui::ProgressBar(progress, { size.x, size.y });
	}

	void gui::progress_bar(float progress, std::string_view overlay_text, const vector2 & size)
	{
		_active_assert();
		ImGui::ProgressBar(progress, { size.x, size.y }, to_string(overlay_text).data());
	}

	void gui::bullet()
	{
		_active_assert();
		ImGui::Bullet();
	}

	bool gui::selectable(std::string_view label, bool selected, selectable_flag flag, const vector2 & size)
	{
		_active_assert();
		return ImGui::Selectable(to_string(label).data(), selected, static_cast<ImGuiSelectableFlags>(flag), { size.x, size.y });
	}

	bool gui::selectable_easy(std::string_view label, bool &selected, selectable_flag flag, const vector2 & size)
	{
		_active_assert();
		return ImGui::Selectable(to_string(label).data(), &selected, static_cast<ImGuiSelectableFlags>(flag), { size.x, size.y });
	}

	bool gui::combo_begin(std::string_view l, std::string_view preview_value, combo_flags f)
	{
		_active_assert();
		return ImGui::BeginCombo(to_string(l).data(), to_string(preview_value).data(), static_cast<ImGuiComboFlags>(f));
	}

	void gui::combo_end()
	{
		_active_assert();
		ImGui::EndCombo();
	}

	bool gui::slider_float(std::string_view label, float& v, float min, float max, slider_flags flags, std::string_view format)
	{
		_active_assert();
		return ImGui::SliderFloat(to_string(label).c_str(), &v, min, max, to_string(format).c_str(), enum_type(flags));
	}

	bool gui::slider_int(std::string_view label, int& v, int min, int max, slider_flags flags, std::string_view format)
	{
		_active_assert();
		return ImGui::SliderInt(to_string(label).c_str(), &v, min, max, to_string(format).c_str(), enum_type(flags));
	}

	bool gui::input_text_multiline(std::string_view label, std::string &buffer, const vector2 &size, input_text_flags f)
	{
		_active_assert();
		return ImGui::InputTextMultiline(to_string(label).data(), &buffer, { size.x, size.y }, static_cast<ImGuiInputTextFlags>(f));
	}

	void gui::colour_editor_options(colour_edit_settings s)
	{
		_active_assert();
		ImGui::SetColorEditOptions(static_cast<ImGuiColorEditFlags>(s));
	}

	bool gui::colour_picker3(std::string_view label, std::array<uint8, 3>& colour, colour_edit_flags f)
	{
		std::array<float, 3> float_col{ 0.f };
		std::transform(std::begin(colour), std::end(colour), std::begin(float_col), [](auto &col)noexcept ->float {
			return col / 255.f;
		});

		const auto r = ImGui::ColorPicker3(to_string(label).data(), float_col.data(), static_cast<ImGuiColorEditFlags>(f));
		if (r)
		{
			std::transform(std::begin(float_col), std::end(float_col), std::begin(colour), [](auto col) noexcept ->uint8 {
				return static_cast<uint8>(col * 255);
			});
		}

		return r;
	}

	bool gui::colour_picker4(std::string_view label, std::array<uint8, 4>& colour, colour_edit_flags f)
	{
		std::array<float, 4> float_col{ 0.f };
		std::transform(std::begin(colour), std::end(colour), std::begin(float_col), [](auto &col)noexcept->float {
			return col / 255.f;
		});

		const auto r = ImGui::ColorPicker4(to_string(label).data(), float_col.data(), static_cast<ImGuiColorEditFlags>(f));
		if (r)
		{
			std::transform(std::begin(float_col), std::end(float_col), std::begin(colour), [](auto col)noexcept->uint8 {
				return static_cast<uint8>(col * 255);
			});
		}

		return r;
	}

	bool gui::collapsing_header(std::string_view s, tree_node_flags f)
	{
		_active_assert();
		return ImGui::CollapsingHeader(to_string(s).data(), static_cast<ImGuiTreeNodeFlags>(f));
	}

	bool gui::collapsing_header(std::string_view s, bool &open, tree_node_flags f)
	{
		_active_assert();
		return ImGui::CollapsingHeader(to_string(s).data(), &open, static_cast<ImGuiTreeNodeFlags>(f));
	}

	bool gui::main_menubar_begin()
	{
		_active_assert();
		const auto r = ImGui::BeginMainMenuBar();
		_main_toolbar_info.main_menubar_y2 = window_position().y + window_size().y;

		return r;
	}

	void gui::main_menubar_end()
	{
		_active_assert();
		ImGui::EndMainMenuBar();
	}

	bool gui::menubar_begin()
	{
		_active_assert();
		return ImGui::BeginMenuBar();
	}

	void gui::menubar_end()
	{
		_active_assert();
		ImGui::EndMenuBar();
	}

	bool gui::menu_begin(std::string_view s, bool enabled)
	{
		_active_assert();
		return ImGui::BeginMenu(to_string(s).data(), enabled);
	}

	void gui::menu_end()
	{
		_active_assert();
		ImGui::EndMenu();
	}

	bool gui::menu_item(std::string_view l, bool enabled)
	{
		_active_assert();
		return ImGui::MenuItem(to_string(l).data(), nullptr, false, enabled);
	}

	bool gui::menu_toggle_item(std::string_view l, bool &selected, bool enabled)
	{
		_active_assert();
		ImGui::MenuItem(to_string(l).data(), nullptr, &selected, enabled);
		return false;
	}

	constexpr auto toolbar_button_size = gui::vector2{ 25.f, 25.f };
	constexpr auto toolbar_layout_size = gui::vector2{ 0.f, -1.f };

	bool gui::main_toolbar_begin()
	{
		_active_assert();
		next_window_position({ 0.f, _main_toolbar_info.main_menubar_y2 });
		next_window_size({ _main_toolbar_info.width, 0.f });

		using namespace std::string_view_literals;
		const auto r = window_begin("##main_toolbar"sv, window_flags::panel);

		//_main_toolbar_info.width = get_item_rect_max().x;
		return r;
	}

	void gui::main_toolbar_end()
	{
		_active_assert();
		window_end();
	}

	bool gui::toolbar_button(std::string_view s)
	{
		_active_assert();
		_toolbar_layout_next();

		const auto str_size = calculate_text_size(s, false);

		//add frame pad
		const auto &pad = ImGui::GetStyle().FramePadding.x * 2;
		const auto result = button(s, {
			std::max(toolbar_button_size.x, str_size.x + pad), toolbar_button_size.y });

		_main_toolbar_info.last_item_x2 = get_item_rect_max().x;
		return result;
	}

	bool gui::toolbar_button(const resources::animation &a)
	{
		_active_assert();
		_toolbar_layout_next();

		const auto result = image_button(a, toolbar_button_size);
		_main_toolbar_info.last_item_x2 = get_item_rect_max().x;
		return result;
	}

	void gui::toolbar_separator()
	{
		_active_assert();
		_toolbar_layout_next();

		separator_horizontal();
		_main_toolbar_info.last_item_x2 = get_item_rect_max().x;
	}

	void gui::tooltip(std::string_view s)
	{
		_active_assert();
		if(is_item_hovered(hovered_flags::none))
			show_tooltip(s);
	}

	void gui::show_tooltip(std::string_view s)
	{
		_active_assert();
		ImGui::SetTooltip(to_string(s).data());
	}

	void gui::open_popup(std::string_view s)
	{
		_active_assert();
		ImGui::OpenPopup(to_string(s).data());
	}

	void gui::popup_begin(std::string_view s)
	{
		_active_assert();
		ImGui::BeginPopup(to_string(s).data());
	}

	void gui::popup_end()
	{
		_active_assert();
		ImGui::EndPopup();
	}

	void gui::close_current_popup()
	{
		_active_assert();
		ImGui::CloseCurrentPopup();
	}

	void gui::open_modal(std::string_view s)
	{
		open_popup(s);
	}

	bool gui::modal_begin(std::string_view s, window_flags flags)
	{
		_active_assert();
		return ImGui::BeginPopupModal(to_string(s).data(), nullptr, static_cast<ImGuiWindowFlags>(flags));
	}

	void gui::modal_end()
	{
		popup_end();
	}

	void gui::close_current_modal()
	{
		close_current_popup();
	}

	void gui::columns_begin(std::size_t count, bool border)
	{
		_active_assert();
		const auto int_count = integer_cast<int>(count);
		ImGui::Columns(int_count, nullptr, border);
	}

	void gui::columns_begin(std::string_view id, std::size_t count, bool border)
	{
		_active_assert();
		const auto int_count = integer_cast<int>(count);
		ImGui::Columns(int_count, to_string(id).data(), border);
	}

	void gui::columns_next()
	{
		_active_assert();
		ImGui::NextColumn();
	}

	void gui::columns_end()
	{
		_active_assert();
		ImGui::Columns();
	}

	void gui::set_keyboard_focus_here(int offset) noexcept
	{
		_active_assert();
		ImGui::SetKeyboardFocusHere(offset);
		return;
	}

	void gui::set_item_default_focus() noexcept
	{
		_active_assert();
		ImGui::SetItemDefaultFocus();
	}

	bool gui::is_item_hovered(hovered_flags f)
	{
		_active_assert();
		return ImGui::IsItemHovered(static_cast<ImGuiHoveredFlags>(f));
	}

	gui::vector2 gui::get_item_rect_max()
	{
		_active_assert();
		const auto v = ImGui::GetItemRectMax();
		return { v.x, v.y };
	}

	gui::vector2 gui::calculate_text_size(std::string_view s, bool include_text_after_double_hash, float wrap_width)
	{
		_active_assert();
		const auto v = ImGui::CalcTextSize(s.data(), s.data() + s.size(), !include_text_after_double_hash, wrap_width);
		return { v.x, v.y };
	}
 
	inline sf::Vertex to_vertex(ImDrawVert vert, vector_float tex_size = { 1.f, 1.f }) noexcept
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

	//NOTE:mixing gl commands in order to get clip clipping scissor glscissor
	// this is done through the draw_clamp_window helper
	void gui::draw(sf::RenderTarget & target, sf::RenderStates states) const
	{
		namespace tex = resources::texture_functions;
		// TODO: to reduce errors, it might be best to simply activate our context
		_active_assert();

		ImGui::Render();
		const auto draw_data = ImGui::GetDrawData();
		assert(draw_data);

		const auto first = draw_data->CmdLists;
		const auto last = draw_data->CmdLists + draw_data->CmdListsCount;

		const auto& view = target.getView();
		const auto view_height = view.getSize().y;

		//for each entry in the draw list
		std::for_each(first, last, [&target, states, view_height](ImDrawList *draw_list) {
			const auto *index_first = draw_list->IdxBuffer.Data;

			//for each command
			for (const auto& cmd : draw_list->CmdBuffer)
			{
				if (cmd.UserCallback)
					std::invoke(cmd.UserCallback, draw_list, &cmd);
				else
				{
					//get the info needed to denormalise the tex coords.
					vector_float texture_size = { 1.f, 1.f };
					if (cmd.TextureId)
					{
						const auto texture = static_cast<const resources::texture*>(cmd.TextureId);
						assert(texture);
						texture_size = static_cast<vector_float>(tex::get_size(texture));
					}

					//get the verts from the draw list that are associated with
					//this command
					sf::VertexArray verts{ sf::Triangles, cmd.ElemCount };
					const auto *i = index_first;
					for (decltype(ImDrawCmd::ElemCount) j = 0; j < cmd.ElemCount; ++j)
						verts[j] = to_vertex(draw_list->VtxBuffer[*(i++)], texture_size);
					
					auto state = states;

					//draw with texture
					if (cmd.TextureId)
					{
						const auto texture = static_cast<const resources::texture*>(cmd.GetTexID());
						assert(texture);
						state.texture = &tex::get_sf_texture(texture);
					}

					const auto clip_region = rect_int{ static_cast<int32>(cmd.ClipRect.x),
						static_cast<int32>(view_height - cmd.ClipRect.w),
						static_cast<int32>(cmd.ClipRect.z - cmd.ClipRect.x),
						static_cast<int32>(cmd.ClipRect.w - cmd.ClipRect.y) };

					//FIXME: should probably be draw_clamp_region; though people are unlikely to move the gui view around
					target.draw(draw_clamp_window{ verts, clip_region }, state);
				}

				//move the index ptr forward for the next command
				index_first += cmd.ElemCount;
			}
		});
	}

	constexpr std::string_view gui::version() noexcept
	{
		return IMGUI_VERSION;
	}

	void gui::_activate_context() noexcept
	{
		assert(_my_context);

		ImGui::SetCurrentContext(_my_context.get());
	}

	void gui::_active_assert() const noexcept
	{
		assert(_my_context);
		assert(ImGui::GetCurrentContext() == _my_context.get());
	}

	void gui::_toolbar_layout_next()
	{
		const auto &style = ImGui::GetStyle();
		//const auto frame_pad = style.FramePadding.x * 2;
		const auto item_spacing = style.ItemSpacing.x;
		
		constexpr auto size = toolbar_button_size.x;
		const auto x2 = _main_toolbar_info.last_item_x2;
		const auto next_button_x2 = size + item_spacing + x2;
		if (next_button_x2 < _main_toolbar_info.width) layout_horizontal(); // layout_horizontal(toolbar_layout_size.x, toolbar_layout_size.y);
	}

	gui::font *gui::_get_font(const resources::font *f)
	{
		assert(f);

		if (const auto font = _fonts.find(f); font != std::end(_fonts))
			return font->second;
		else
			return _create_font(f);
	}

	gui::font *gui::_create_font(const resources::font *f)
	{
		auto &f_atlas = *_font_atlas;
		ImFontConfig cfg;
		cfg.FontDataOwnedByAtlas = false;
		//const cast, because f_atlas demands control of the ptr
		//though it won't actually do anything, since FontDataOwned is set to false
		const auto size = f->source_buffer.size();
		const auto int_size = integer_cast<int>(size);
		const auto out = f_atlas.AddFontFromMemoryTTF(const_cast<std::byte*>(f->source_buffer.data()), int_size, 13.f, &cfg);
		_generate_atlas();

		return out;
	}

	void gui::_generate_atlas()
	{
		namespace tex = resources::texture_functions;
		//get texture
		auto [d, lock] = data::detail::get_data_manager_exclusive_lock();
		std::ignore = lock;

		static unique_id font_texture_id{};

		auto t = tex::find_create_texture(*d, font_texture_id, unique_id::zero);

		if (tex::get_is_loaded(t))
			return;

		//get the data and set the correct ids
		auto& f_atlas = *_font_atlas;
		int width = 0, height = 0;
		unsigned char* texture_data = nullptr;
		f_atlas.GetTexDataAsRGBA32(&texture_data, &width, &height);
		f_atlas.SetTexID(t);

		//make the texture
		auto& texture = tex::get_sf_texture(t);
		texture.create(width, height);
		texture.update(texture_data);
		//apply correct settings
		tex::set_settings(t,
			{ integer_cast<texture_size_t>(width), integer_cast<texture_size_t>(height) },
			false, //smooth
			false, //repeat 
			false, //mips
			true); //set loaded
	}

	//gui::static objects
	std::unique_ptr<ImFontAtlas> gui::_font_atlas{ nullptr };
	std::unordered_map<const resources::font*, gui::font*> gui::_fonts;

	gui_input_text_callback::gui_input_text_callback(ImGuiInputTextCallbackData* data) noexcept
		: _data{ data }
	{}

	gui::input_text_flags gui_input_text_callback::get_callback_trigger() const noexcept
	{
		return gui::input_text_flags{ _data->EventFlag };
	}

	gui::input_text_flags gui_input_text_callback::get_input_flags() const noexcept
	{
		return gui::input_text_flags{ _data->Flags };
	}

	ImWchar gui_input_text_callback::get_char_filter_input() const noexcept
	{
		return _data->EventChar;
	}

	void gui_input_text_callback::set_char_filter_replacement(ImWchar ch) noexcept
	{
		_data->EventChar = ch;
		return;
	}

	gui_input_text_callback::input_key gui_input_text_callback::get_input_key() const noexcept
	{
		return input_key{ _data->EventKey };
	}

	std::string_view gui_input_text_callback::input_contents() const noexcept
	{
		return std::string_view{ _data->Buf,
			integer_cast<std::string_view::size_type>(_data->BufTextLen) };
	}

	int gui_input_text_callback::cursor_pos() const noexcept
	{
		return _data->CursorPos;
	}

	void gui_input_text_callback::set_cursor_pos(int pos) noexcept
	{
		_data->CursorPos = pos;
		return;
	}

	bool gui_input_text_callback::has_selection() const noexcept
	{
		return _data->HasSelection();
	}

	std::pair<int, int> gui_input_text_callback::selection_range() const noexcept
	{
		return { _data->SelectionStart, _data->SelectionEnd };
	}

	void gui_input_text_callback::set_selection_range(int start, int end) noexcept
	{
		_data->SelectionStart = start;
		_data->SelectionEnd = end;
		return;
	}

	void gui_input_text_callback::erase_chars(int pos, int count) noexcept
	{
		_data->DeleteChars(pos, count);
		return;
	}

	void gui_input_text_callback::clear_chars() noexcept
	{
		_data->DeleteChars(0, _data->BufTextLen);
		return;
	}

	void gui_input_text_callback::insert_chars(int pos, std::string_view text)
	{
		_data->InsertChars(pos, text.data(), text.data() + text.size());
		return;
	}

	void gui_input_text_callback::select_all() noexcept
	{
		_data->SelectAll();
		return;
	}

	void gui_input_text_callback::clear_selection() noexcept
	{
		_data->ClearSelection();
		return;
	}
}

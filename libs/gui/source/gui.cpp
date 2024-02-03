#include "hades/gui.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include "SFML/OpenGL.hpp"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/Graphics/VertexArray.hpp"

#include "SFML/System/Err.hpp"

#include "hades/animation.hpp"
#include "hades/colour.hpp"
#include "hades/data.hpp"
#include "hades/draw_tools.hpp"
#include "hades/font.hpp"
#include "hades/standard_paths.hpp"
#include "hades/texture.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace hades
{
	void detail::gui_context_deleter::operator()(gui_context* c) const noexcept
	{
		ImGui::DestroyContext(c);
		return;
	}

	static std::once_flag make_default_font;

	constexpr auto default_config_name = "ui.ini"sv;

	gui::gui()
		: gui{ default_config_name }
	{}

	gui::gui(std::string_view filename)
		: _my_context{ {}, {} }
	{
		if (!_font_atlas)
			_font_atlas = std::make_unique<ImFontAtlas>();
		assert(_font_atlas);

		const auto prev_context = ImGui::GetCurrentContext();

		_my_context = context_ptr{ ImGui::CreateContext(_font_atlas.get()) };
		assert(_my_context);
		
		_activate_context(); //context will only be activated by default if it is the only one

		std::call_once(make_default_font, _create_default_font);

		// save config path for this gui
		auto &io = ImGui::GetIO();
		const auto path = user_config_directory() / (empty(filename) ? default_config_name : filename);
		if (const auto directory = path.parent_path(); !std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);
		else if (!std::filesystem::is_directory(directory))
			log_error("Unable to create directory for ui config: "s + std::filesystem::absolute(directory).generic_string());

		assert(path.extension() == ".ini");
		_ini_filename = path.generic_string();
		io.IniFilename = _ini_filename.c_str();

		using flags = ImGuiBackendFlags_;
		io.BackendFlags |= flags::ImGuiBackendFlags_RendererHasVtxOffset;
		// TODO: ImGuiBackendFlags_HasMouseCursors
		//			_SetCursorPos
		io.BackendPlatformName = "hades"; //TODO: this should be game_name()
		io.BackendRendererName = "hades_SFML";
		
		// TODO: clipboard support

		set_display_size({ 1.f, 1.f });

		frame_begin();
		frame_end();

		ImGui::SetCurrentContext(prev_context);
	}

	void gui::activate_context() noexcept
	{
		_activate_context();
	}

	void gui::set_display_size(vector2 size)
	{
		_active_assert();
		auto &io = ImGui::GetIO();
		io.DisplaySize = { size.x, size.y };

		_main_toolbar_info.width = size.x;
	}

	static constexpr ImGuiKey to_imgui_key(sf::Keyboard::Key k) noexcept
	{
		switch(k)
		{
		case sf::Keyboard::Tab: return ImGuiKey_Tab;
		case sf::Keyboard::Left: return ImGuiKey_LeftArrow;
		case sf::Keyboard::Right: return ImGuiKey_RightArrow;
		case sf::Keyboard::Up: return ImGuiKey_UpArrow;
		case sf::Keyboard::Down: return ImGuiKey_DownArrow;
		case sf::Keyboard::PageUp: return ImGuiKey_PageUp;
		case sf::Keyboard::PageDown: return ImGuiKey_PageDown;
		case sf::Keyboard::Home: return ImGuiKey_Home;
		case sf::Keyboard::End: return ImGuiKey_End;
		case sf::Keyboard::Insert: return ImGuiKey_Insert;
		//see sfml_imgui for android bindings
		case sf::Keyboard::Delete: return ImGuiKey_Delete;
		case sf::Keyboard::Backspace: return ImGuiKey_Backspace;
		case sf::Keyboard::Space: return ImGuiKey_Space;
		case sf::Keyboard::Enter: return ImGuiKey_Enter;
		case sf::Keyboard::Escape: return ImGuiKey_Escape;
		case sf::Keyboard::A: return ImGuiKey_A;
		case sf::Keyboard::C: return ImGuiKey_C;
		case sf::Keyboard::V: return ImGuiKey_V;
		case sf::Keyboard::X: return ImGuiKey_X;
		case sf::Keyboard::Y: return ImGuiKey_Y;
		case sf::Keyboard::Z: return ImGuiKey_Z;
		default: return ImGuiKey_COUNT;
		}
	}

	// Custom output iterator for SFML algorithm
	// performs like a normal random access iterator
	template<typename Container, typename Iter>
	class special_iterator
	{
	public:
		using container_type = Container; // required by SFML for some reason, only certain iterators support this
		using value_type = Iter::value_type;

		constexpr special_iterator(const Container&, Iter iter) noexcept
			: _iter{ iter } {}

		template<typename T>
			requires std::same_as<value_type, std::decay_t<T>>
		constexpr special_iterator& operator=(T&& val) 
		{
			*_iter = std::forward<T>(val);
			return *this;
		}

		[[nodiscard]] 
		constexpr value_type& operator*() noexcept 
		{
			return *_iter;
		}

		constexpr Iter operator++(int) noexcept
		{
			auto i = _iter++;
			return i;
		}

	protected:
		Iter _iter;
	};

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
			io.AddMouseSourceEvent(ImGuiMouseSource::ImGuiMouseSource_Mouse);
			io.AddMousePosEvent(static_cast<float>(e.mouseMove.x), static_cast<float>(e.mouseMove.y));
			return io.WantCaptureMouse;
		case sf::Event::MouseButtonPressed:
			[[fallthrough]];
		case sf::Event::MouseButtonReleased:
			io.AddMouseSourceEvent(ImGuiMouseSource::ImGuiMouseSource_Mouse);
			io.AddMouseButtonEvent(enum_type(e.mouseButton.button), e.type == sf::Event::MouseButtonPressed);
			return io.WantCaptureMouse;
		case sf::Event::MouseWheelScrolled:
			io.AddMouseSourceEvent(ImGuiMouseSource::ImGuiMouseSource_Mouse);
			if (e.mouseWheelScroll.wheel == sf::Mouse::Wheel::Vertical)
				io.AddMouseWheelEvent({}, static_cast<float>(e.mouseWheelScroll.delta));
			else
				io.AddMouseWheelEvent(static_cast<float>(e.mouseWheelScroll.delta), {});
			return io.WantCaptureMouse;
		case sf::Event::KeyPressed:
			[[fallthrough]];
		case sf::Event::KeyReleased:
		{
			const auto k = to_imgui_key(e.key.code);
			if (k != ImGuiKey_COUNT)
			{
				io.AddKeyEvent(k, e.type == sf::Event::KeyPressed);
				io.AddKeyEvent(ImGuiMod_Ctrl, e.key.control);
				io.AddKeyEvent(ImGuiMod_Shift, e.key.shift);
				io.AddKeyEvent(ImGuiMod_Alt, e.key.alt);
				io.AddKeyEvent(ImGuiMod_Super, e.key.system);
			}
			return io.WantCaptureKeyboard;
		}
		case sf::Event::TextEntered:
			if (e.text.unicode == '\b') // Manually insert backspace keys, 
			{							// otherwise imgui won't allow repeats
				io.AddKeyEvent(to_imgui_key(sf::Keyboard::Backspace), true); //keydown
				io.AddKeyEvent(to_imgui_key(sf::Keyboard::Backspace), false); // keyup
			}
			else if (e.text.unicode < 0x10000) 
			{
				auto u8_buffer = std::array<std::uint8_t, 5>{}; // maximum of 4 encoded bytes, plus a null byte
				u8_buffer.fill('\0');
				// unicode encoder requires special iterator wrapper to provide ::container_type
				sf::Utf8::encode(e.text.unicode, special_iterator{ u8_buffer, begin(u8_buffer) });
				io.AddInputCharactersUTF8(reinterpret_cast<const char*>(u8_buffer.data()));
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

	void gui::create_font(const resources::font* f)
	{
		_create_font(f);
		return;
	}


	void gui::frame_begin()
	{
		_active_assert();
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

	void gui::show_vertex_test_window()
	{
		_active_assert();
		if (!ImGui::Begin("Dear ImGui Backend Checker"))
		{
			ImGui::End();
			return;
		}

		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Dear ImGui %s Backend Checker", ImGui::GetVersion());
		ImGui::Text("io.BackendPlatformName: %s", io.BackendPlatformName ? io.BackendPlatformName : "NULL");
		ImGui::Text("io.BackendRendererName: %s", io.BackendRendererName ? io.BackendRendererName : "NULL");
		ImGui::Separator();

		if (ImGui::TreeNode("0001: Renderer: Large Mesh Support"))
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			{
				static int vtx_count = 60000;
				ImGui::SliderInt("VtxCount##1", &vtx_count, 0, 100000);
				ImVec2 p = ImGui::GetCursorScreenPos();
				for (int n = 0; n < vtx_count / 4; n++)
				{
					float off_x = (float)(n % 100) * 3.0f;
					float off_y = (float)(n % 100) * 1.0f;
					ImU32 col = IM_COL32(((n * 17) & 255), ((n * 59) & 255), ((n * 83) & 255), 255);
					draw_list->AddRectFilled(ImVec2(p.x + off_x, p.y + off_y), ImVec2(p.x + off_x + 50, p.y + off_y + 50), col);
				}
				ImGui::Dummy(ImVec2(300 + 50, 100 + 50));
				ImGui::Text("VtxBuffer.Size = %d", draw_list->VtxBuffer.Size);
			}
			{
				static int vtx_count = 60000;
				ImGui::SliderInt("VtxCount##2", &vtx_count, 0, 100000);
				ImVec2 p = ImGui::GetCursorScreenPos();
				for (int n = 0; n < vtx_count / (10 * 4); n++)
				{
					float off_x = (float)(n % 100) * 3.0f;
					float off_y = (float)(n % 100) * 1.0f;
					ImU32 col = IM_COL32(((n * 17) & 255), ((n * 59) & 255), ((n * 83) & 255), 255);
					draw_list->AddText(ImVec2(p.x + off_x, p.y + off_y), col, "ABCDEFGHIJ");
				}
				ImGui::Dummy(ImVec2(300 + 50, 100 + 20));
				ImGui::Text("VtxBuffer.Size = %d", draw_list->VtxBuffer.Size);
			}
			ImGui::TreePop();
		}

		ImGui::End();
	}

	bool gui::window_begin(std::string_view name, bool &closed, window_flags flags)
	{
		_active_assert();
		return ImGui::Begin(name, &closed, static_cast<ImGuiWindowFlags>(flags));
	}

	bool gui::window_begin(std::string_view name, window_flags flags)
	{
		_active_assert();
		return ImGui::Begin(name, nullptr, static_cast<ImGuiWindowFlags>(flags));;
	}

	void gui::window_end()
	{
		_active_assert();
		ImGui::End();
	}

	bool gui::child_window_begin(std::string_view name, vector2 size, bool border, window_flags flags)
	{
		_active_assert();
		return ImGui::BeginChild(name, { size.x, size.y }, border, static_cast<ImGuiWindowFlags>(flags));
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

	void gui::next_window_size(vector2 s, set_condition_enum cond)
	{
		_active_assert();
		ImGui::SetNextWindowSize({ s.x, s.y }, enum_type(cond));
		return;
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

	template<typename Colour>
	static inline ImVec4 to_imvec4(const Colour &c) noexcept
	{
		return { c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f };
	}

	void gui::push_colour(colour_target element, const sf::Color &c)
	{
		_active_assert();
		ImGui::PushStyleColor(static_cast<ImGuiCol>(element), to_imvec4(c));
	}

	void gui::push_colour(colour_target element, const colour& c)
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

	void gui::same_line(float pos, float width)
	{
		_active_assert();
		ImGui::SameLine(pos, width);
	}

	void gui::same_line_wrapping(const float next_item_size_x, const float pos, const float width)
	{
		_active_assert();
		const auto& style = ImGui::GetStyle();
		const auto window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		auto last_button_x2 = ImGui::GetItemRectMax().x;
		// expected position if next button was on same line
		auto next_button_x2 = last_button_x2 + style.ItemSpacing.x + next_item_size_x; 
		if (next_button_x2 < window_visible_x2)
			ImGui::SameLine(pos, width);

		return;
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

	void gui::dummy(vector2_float s)
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
        ImGui::PushID(s);
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
		ImGui::TextUnformatted(s);
	}

	void gui::text_coloured(std::string_view s, const sf::Color &c)
	{
		_active_assert();
		push_colour(colour_target::text, c);
		text(s);
		pop_colour();
	}

	void gui::text_coloured(std::string_view s, const colour& c)
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
		ImGui::PushTextWrapPos();
        ImGui::TextUnformatted(s);
		ImGui::PopTextWrapPos();
		return;
	}

	void gui::text_bullet(std::string_view s)
	{
		_active_assert();
		bullet();
		text(s);
	}

	void gui::separator_text(std::string_view s)
	{
		_active_assert();
		return ImGui::SeparatorText(s);
	}

	bool gui::button(std::string_view label, const vector2 &size)
	{
		_active_assert();
		return ImGui::Button(label, { size.x, size.y });
	}

	bool gui::small_button(std::string_view label)
	{
		_active_assert();
		return ImGui::SmallButton(label);
	}

	bool gui::invisible_button(std::string_view label, const vector2 & size)
	{
		_active_assert();
		return ImGui::InvisibleButton(label, { size.x, size.y });
	}

	bool gui::arrow_button(std::string_view label, direction d)
	{
		_active_assert();
		return ImGui::ArrowButton(label, static_cast<ImGuiDir>(d));
	}

	void gui::image(const sf::Texture& t, const rect_float& text_coords, const vector2& size, const sf::Color& tint_colour, const sf::Color& border_colour)
	{
		_active_assert();

		const auto x = text_coords.x,
			y = text_coords.y,
			width = text_coords.width,
			height = text_coords.height;

		const auto [tex_width, tex_height] = t.getSize();

		ImGui::Image(&t,
			{ size.x, size.y },
			{ x / tex_width, y / tex_height }, // normalised coords
			{ (x + width) / tex_width,  (y + height) / tex_height }, // absolute pos for bottom right corner, also normalised
			to_imvec4(tint_colour),
			to_imvec4(border_colour));
	}


	void gui::image(const resources::texture &t, const rect_float &text_coords, const vector2 &size, const sf::Color &tint_colour, const sf::Color &border_colour)
	{
		_active_assert();
		image(resources::texture_functions::get_sf_texture(&t), text_coords, size, tint_colour, border_colour);
		return;
	}

	void gui::image(const resources::animation &a, const vector2 size, const time_point time,
		const sf::Color &tint_colour, const sf::Color &border_colour)
	{		
		const auto& f = animation::get_frame(a, time);
		_image_animation_frame(a, size, f, tint_colour, border_colour);
		return;
	}

	void gui::image(const resources::animation& a, const vector2 size, const int32 frame,
		const sf::Color& tint_colour, const sf::Color& border_colour)
	{
		const auto& f = animation::get_frame(a, frame);
		_image_animation_frame(a, size, f, tint_colour, border_colour);
		return;
	}

	bool gui::image_button(std::string_view id, const resources::texture &texture,
		const rect_float &text_coords, const vector2 &size, const sf::Color &background_colour,
		const sf::Color &tint_colour)
	{
		_active_assert();

		const auto x = text_coords.x,
			y = text_coords.y,
			width = text_coords.width,
			height = text_coords.height;

		namespace tex = resources::texture_functions;
		const auto [tex_width, tex_height] = tex::get_size(texture);

		return ImGui::ImageButton(id, &tex::get_sf_texture(&texture),
			{ size.x, size.y },
			{ x / tex_width, y / tex_height }, // normalised coords
			{ (x + width) / tex_width,  (y + height) / tex_height }, // absolute pos for bottom right corner, also normalised
			to_imvec4(background_colour),
			to_imvec4(tint_colour));
	}

	bool gui::image_button(std::string_view id, const resources::animation &a, const vector2 &size, time_point time, const sf::Color &background_colour, const sf::Color &tint_colour)
	{
		_active_assert();
		const auto& f = animation::get_frame(a, time);
		const auto texture = resources::animation_functions::get_texture(a);
		assert(texture);

		const auto flip_x = f.scale_w < 0 || f.w < 0;
		const auto flip_y = f.scale_h < 0 || f.h < 0;

		const auto [tex_width, tex_height] = resources::texture_functions::get_size(*texture);

		auto uv0 = ImVec2{ f.x / tex_width, f.y / tex_height };
		auto uv1 = ImVec2{ (f.x + abs(f.w)) / tex_width, (f.y + abs(f.h)) / tex_height };

		if (flip_x)
			std::swap(uv0.x, uv1.x);
		if (flip_y)
			std::swap(uv0.y, uv1.y);

		return ImGui::ImageButton(id, &resources::texture_functions::get_sf_texture(texture),
			{ size.x, size.y }, uv0, uv1,
			to_imvec4(background_colour),
			to_imvec4(tint_colour));
	}

	bool gui::checkbox(std::string_view label, bool &checked)
	{
		_active_assert();
		return ImGui::Checkbox(label, &checked);
	}

	bool gui::radio_button(std::string_view label, bool active)
	{
		_active_assert();
		return ImGui::RadioButton(label, active);
	}

	void gui::progress_bar(float progress, const vector2 &size)
	{
		_active_assert();
		ImGui::ProgressBar(progress, { size.x, size.y });
	}

	void gui::progress_bar(float progress, std::string_view overlay_text, const vector2 & size)
	{
		_active_assert();
		ImGui::ProgressBar(progress, { size.x, size.y }, overlay_text);
	}

	void gui::bullet()
	{
		_active_assert();
		ImGui::Bullet();
	}

	gui::vector2 gui::calculate_button_size(std::string_view label, const gui::vector2& size)
	{
		_active_assert();
		const auto& style = ImGui::GetStyle();
		const auto label_size = ImGui::CalcTextSize(label);
		const auto ret = ImGui::CalcItemSize({ size.x, size.y },
			label_size.x + style.FramePadding.x * 2.0f,
			label_size.y + style.FramePadding.y * 2.0f);

		return { ret.x, ret.y };
	}

	bool gui::selectable(std::string_view label, bool selected, selectable_flag flag, const vector2 & size)
	{
		_active_assert();
		return ImGui::Selectable(label, selected, static_cast<ImGuiSelectableFlags>(flag), { size.x, size.y });
	}

	bool gui::selectable_easy(std::string_view label, bool &selected, selectable_flag flag, const vector2 & size)
	{
		_active_assert();
		return ImGui::Selectable(label, &selected, static_cast<ImGuiSelectableFlags>(flag), { size.x, size.y });
	}

	bool gui::combo_begin(std::string_view l, std::string_view preview_value, combo_flags f)
	{
		_active_assert();
		return ImGui::BeginCombo(l, preview_value, static_cast<ImGuiComboFlags>(f));
	}

	void gui::combo_end()
	{
		_active_assert();
		ImGui::EndCombo();
	}

	bool gui::listbox_begin(std::string_view label, const vector2& size)
	{
		_active_assert();
		return ImGui::BeginListBox(label, { size.x, size.y });
	}

	void gui::listbox_end()
	{
		_active_assert();
		ImGui::EndListBox();
		return;
	}

	bool gui::slider_float(std::string_view label, float& v, float min, float max, slider_flags flags, std::string_view format)
	{
		_active_assert();
		// FIXME: Format params don't support string_view
		return ImGui::SliderFloat(label, &v, min, max, to_string(format).c_str(), enum_type(flags));
	}

	bool gui::slider_int(std::string_view label, int& v, int min, int max, slider_flags flags, std::string_view format)
	{
		_active_assert();
		// FIXME: Format params don't support string_view
		return ImGui::SliderInt(label, &v, min, max, to_string(format).c_str(), enum_type(flags));
	}

	bool gui::input_text_multiline(std::string_view label, std::string &buffer, const vector2 &size, input_text_flags f)
	{
		_active_assert();
		// FIXME: imgui_stdlib.h doesn't support ImStrv yet 
		return ImGui::InputTextMultiline(to_string(label).data(), &buffer, {size.x, size.y}, static_cast<ImGuiInputTextFlags>(f));
	}

	void gui::colour_editor_options(colour_edit_flags s)
	{
		_active_assert();
		ImGui::SetColorEditOptions(static_cast<ImGuiColorEditFlags>(s));
	}

	bool gui::colour_edit(std::string_view label, colour& c, colour_edit_flags flags)
	{
		std::array<float, 4> f{
			c.r / 255.f,
			c.g / 255.f,
			c.b / 255.f,
			c.a / 255.f
		};

		const auto r = ImGui::ColorEdit4(label, f.data(), static_cast<ImGuiColorEditFlags>(flags));
		if (r)
		{
			c = colour{
				integral_cast<uint8>(f[0] * 255),
				integral_cast<uint8>(f[1] * 255),
				integral_cast<uint8>(f[2] * 255),
				integral_cast<uint8>(f[3] * 255)
			};
		}

		return r;
	}

	bool gui::colour_picker(std::string_view label, std::array<uint8, 3>& colour, colour_edit_flags f)
	{
		std::array<float, 3> float_col{ 0.f };
		std::transform(std::begin(colour), std::end(colour), std::begin(float_col), [](auto &col)noexcept ->float {
			return col / 255.f;
		});

		const auto r = ImGui::ColorPicker3(label, float_col.data(), static_cast<ImGuiColorEditFlags>(f));
		if (r)
		{
			std::transform(std::begin(float_col), std::end(float_col), std::begin(colour), [](auto col) noexcept ->uint8 {
				return static_cast<uint8>(col * 255);
			});
		}

		return r;
	}

	bool gui::colour_picker(std::string_view label, std::array<uint8, 4>& colour, colour_edit_flags f)
	{
		std::array<float, 4> float_col{ 0.f };
		std::transform(std::begin(colour), std::end(colour), std::begin(float_col), [](auto &col)noexcept->float {
			return col / 255.f;
		});

		const auto r = ImGui::ColorPicker4(label, float_col.data(), static_cast<ImGuiColorEditFlags>(f));
		if (r)
		{
			std::transform(std::begin(float_col), std::end(float_col), std::begin(colour), [](auto col)noexcept->uint8 {
				return static_cast<uint8>(col * 255);
			});
		}

		return r;
	}

	bool gui::colour_picker(std::string_view label, colour& c, colour_edit_flags flags)
	{
		std::array<float, 4> f{ 
			c.r / 255.f,
			c.g / 255.f,
			c.b / 255.f,
			c.a / 255.f
		};

		const auto r = ImGui::ColorPicker4(label, f.data(), static_cast<ImGuiColorEditFlags>(flags));
		if (r)
		{
			c = colour{
				integral_cast<uint8>(f[0] * 255),
				integral_cast<uint8>(f[1] * 255),
				integral_cast<uint8>(f[2] * 255),
				integral_cast<uint8>(f[3] * 255)
			};
		}

		return r;
	}

	bool gui::tree_node(std::string_view sv, tree_node_flags tn)
	{
		_active_assert();
		return ImGui::TreeNodeEx(string{ sv }.data(), enum_type(tn));
	}

	void gui::tree_pop()
	{
		_active_assert();
		ImGui::TreePop();
	}

	bool gui::collapsing_header(std::string_view s, tree_node_flags f)
	{
		_active_assert();
		return ImGui::CollapsingHeader(s, static_cast<ImGuiTreeNodeFlags>(f));
	}

	bool gui::collapsing_header(std::string_view s, bool &open, tree_node_flags f)
	{
		_active_assert();
		return ImGui::CollapsingHeader(s, &open, static_cast<ImGuiTreeNodeFlags>(f));
	}

	void gui::set_next_item_open(bool is_open, set_condition_enum e)
	{
		_active_assert();
		ImGui::SetNextItemOpen(is_open, enum_type(e));
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
		return ImGui::BeginMenu(s, enabled);
	}

	void gui::menu_end()
	{
		_active_assert();
		ImGui::EndMenu();
	}

	bool gui::menu_item(std::string_view l, bool enabled)
	{
		_active_assert();
		return ImGui::MenuItem(l, nullptr, false, enabled);
	}

	bool gui::menu_toggle_item(std::string_view l, bool &selected, bool enabled)
	{
		_active_assert();
		ImGui::MenuItem(l, nullptr, &selected, enabled);
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
		const auto r = window_begin("##main_toolbar"sv, window_flags::panel | window_flags::no_bring_to_front_on_focus);
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
		const auto str_size = calculate_text_size(s, false);
		const auto& pad = ImGui::GetStyle().FramePadding.x * 2;
		const auto button_size = vector2{
			std::max(toolbar_button_size.x, str_size.x + pad), toolbar_button_size.y };
		const auto size = calculate_button_size(s, button_size);
		same_line_wrapping(size.x);
		return button(s, button_size);
	}

	bool gui::toolbar_button(std::string_view id, const resources::animation &a)
	{
		_active_assert();
		const auto size = calculate_button_size({}, toolbar_button_size);
		same_line_wrapping(size.x);
		return image_button(id, a, toolbar_button_size);
	}

	void gui::toolbar_separator()
	{
		_active_assert();
		const auto size = calculate_button_size({}, toolbar_button_size);
		same_line_wrapping(size.x);
		separator_horizontal();
	}

	bool gui::tooltip_begin()
	{
		_active_assert();
		return ImGui::BeginTooltip();
	}

	void gui::tooltip_end()
	{
		_active_assert();
		ImGui::EndTooltip();
		return;
	}

	// TODO: these functions that pass through to fmt strings
	//		need to go back to accepting const string&
	void gui::tooltip(std::string_view s)
	{
		_active_assert();
		ImGui::SetItemTooltip(to_string(s).c_str()); // FIXME: fmt strings aren't string view
        return;
	}

	void gui::open_popup(std::string_view s)
	{
		_active_assert();
		ImGui::OpenPopup(s);
	}

	void gui::popup_begin(std::string_view s)
	{
		_active_assert();
		ImGui::BeginPopup(s);
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
		return ImGui::BeginPopupModal(s, nullptr, static_cast<ImGuiWindowFlags>(flags));
	}

	void gui::modal_end()
	{
		popup_end();
	}

	void gui::close_current_modal()
	{
		close_current_popup();
	}

	bool gui::begin_table(std::string_view str_id, int column, table_flags flags,
		const vector2& outer_size, float inner_width)
	{
		_active_assert();
		return ImGui::BeginTable(str_id, column, enum_type(flags),
			{ outer_size.x, outer_size.y }, inner_width);
	}

	void gui::end_table()
	{
		_active_assert();
		ImGui::EndTable();
		return;
	}

	void gui::table_setup_column(std::string_view label, table_column_flags flags, float init_width_or_weight/*, ImGuiID user_id*/)
	{
		_active_assert();
		ImGui::TableSetupColumn(label, enum_type(flags), init_width_or_weight);
		return;
	}

	void gui::table_headers_row()
	{
		_active_assert();
		ImGui::TableHeadersRow();
		return;
	}

	void gui::table_next_row(table_row_flags row_flags, float min_row_height)
	{
		_active_assert();
		ImGui::TableNextRow(enum_type(row_flags), min_row_height);
		return;
	}

	bool gui::table_next_column()
	{
		_active_assert();
		return ImGui::TableNextColumn();
	}

	bool gui::table_set_column_index(int column_n)
	{
		_active_assert();
		return ImGui::TableSetColumnIndex(column_n);
	}

	bool gui::table_needs_sort() const noexcept
	{
		_active_assert();
		auto specs = ImGui::TableGetSortSpecs();
		auto ret = specs && specs->SpecsDirty && specs->SpecsCount;
		if (ret)
			specs->SpecsDirty = false;
		return ret;
	}

	gui::sort_column_return gui::table_sort_column(int sort_order) const noexcept
	{
		auto specs = ImGui::TableGetSortSpecs();
		assert(specs);
		assert(specs->Specs);
		assert(specs->SpecsCount > sort_order);

		auto& entry = *(specs->Specs + sort_order);
		return { entry.ColumnIndex, sort_direction{ entry.SortDirection } };
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
		ImGui::Columns(int_count, id, border);
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

	bool gui::begin_dragdrop_source(dragdrop_flags flags)
	{
		_active_assert();
		return ImGui::BeginDragDropSource(enum_type(flags));
	}

	void gui::end_dragdrop_source()
	{
		_active_assert();
		return ImGui::EndDragDropSource();
	}

	bool gui::begin_dragdrop_target()
	{
		_active_assert();
		return ImGui::BeginDragDropTarget();
	}

	void gui::end_dragdrop_target()
	{
		_active_assert();
		ImGui::EndDragDropTarget();
		return;
	}

	void gui::begin_disabled()
	{
		_active_assert();
		ImGui::BeginDisabled();
		return;
	}

	void gui::end_disabled()
	{
		_active_assert();
		ImGui::EndDisabled();
		return;
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

	gui::vector2 gui::get_mouse_pos() const
	{
		_active_assert();
		const auto& io = ImGui::GetIO();
		return { io.MousePos.x, io.MousePos.y };
	}

	bool gui::is_item_hovered(hovered_flags f)
	{
		_active_assert();
		return ImGui::IsItemHovered(static_cast<ImGuiHoveredFlags>(f));
	}

	bool gui::is_item_focused()
	{
		_active_assert();
		return ImGui::IsItemFocused();
	}

	bool gui::is_item_clicked(mouse_button mb)
	{
		_active_assert();
		return ImGui::IsItemClicked(enum_type(mb));
	}

	bool gui::is_item_edited()
	{
		_active_assert();
		return ImGui::IsItemEdited();
	}


	bool gui::is_item_toggled_open()
	{
		_active_assert();
		return ImGui::IsItemToggledOpen();
	}

	gui::vector2 gui::get_item_rect_min()
	{
		_active_assert();
		const auto v = ImGui::GetItemRectMin();
		return { v.x, v.y };
	}

	gui::vector2 gui::get_item_rect_max()
	{
		_active_assert();
		const auto v = ImGui::GetItemRectMax();
		return { v.x, v.y };
	}

	gui::vector2 gui::get_item_rect_size()
	{
		_active_assert();
		const auto v = ImGui::GetItemRectSize();
		return { v.x, v.y };
	}

	gui::vector2 gui::calculate_text_size(std::string_view s, bool include_text_after_double_hash, float wrap_width)
	{
		_active_assert();
		const auto v = ImGui::CalcTextSize(s, !include_text_after_double_hash, wrap_width);
		return { v.x, v.y };
	}
 
	static inline constexpr sf::Vertex to_vertex(ImDrawVert vert, vector2_float tex_size = { 1.f, 1.f }) noexcept
	{
		const auto col = ImColor{ vert.col }.Value;

		return { {vert.pos.x, vert.pos.y},
			sf::Color{static_cast<std::uint8_t>(col.x * 255.f),
					static_cast<std::uint8_t>(col.y * 255.f),
					static_cast<std::uint8_t>(col.z * 255.f),
					static_cast<std::uint8_t>(col.w * 255.f)},
			//uv coords are normalised for the texture size as [0.f, 1.f]
			//we need to expand them to the range of [0, tex_size]
			//same as the colours above
			{vert.uv.x * tex_size.x, vert.uv.y * tex_size.y} };
	}

	namespace
	{
		class vector_drawable : public sf::Drawable
		{
		public:
			vector_drawable(std::vector<sf::Vertex>& v, sf::PrimitiveType p) noexcept
				: _vect{ v }, _prim{ p } {}

			void draw(sf::RenderTarget& target, const sf::RenderStates& states) const override
			{
				target.draw(_vect.data(), size(_vect), _prim, states);
				return;
			}
		private:
			const std::vector<sf::Vertex>& _vect;
			const sf::PrimitiveType _prim;
		};
	}

	//NOTE: mixing gl commands in order to get clip clipping scissor glscissor
	// this is done through the draw_clamp_region helper
	void gui::draw(sf::RenderTarget & target, const sf::RenderStates &states) const
	{
		namespace tex = resources::texture_functions;
		// TODO: to reduce errors, it might be best to simply activate our context
		_active_assert();

		ImGui::Render();
		const auto draw_data = ImGui::GetDrawData();
		assert(draw_data);

		const auto& view = target.getView();
		const auto view_height = view.getSize().y;

		auto vertex_array = std::vector<sf::Vertex>{};

		//for each entry in the draw list
		std::ranges::for_each(draw_data->CmdLists, [&target, &vertex_array, draw_data, states, view_height](ImDrawList* draw_list) {
			const auto index_first = draw_list->IdxBuffer.Data;

			//for each command
			for (const auto& cmd : draw_list->CmdBuffer)
			{
				if (cmd.UserCallback)
					std::invoke(cmd.UserCallback, draw_list, &cmd);
				else
				{
					//get the info needed to denormalise the tex coords.
					//	used in to_vertex
					vector2_float texture_size = { 1.f, 1.f };
					if (cmd.TextureId)
					{
						const auto texture = cmd.TextureId;
						assert(texture);
						const auto size = texture->getSize();
						texture_size = vector2_float{ float_cast(size.x), float_cast(size.y) };
					}

					//get the verts from the draw list that are associated with
					//this command
					vertex_array.clear();
					vertex_array.reserve(cmd.ElemCount);
					const auto v_offset = cmd.VtxOffset;
					const auto index_begin = std::next(index_first, cmd.IdxOffset);
					const auto index_end = std::next(index_begin, cmd.ElemCount);
					std::transform(index_begin, index_end, back_inserter(vertex_array), 
						[&](const auto index) {
                        // NOTE: sign cast the index calculation, ImVector uses signed indexes.
                        return to_vertex(draw_list->VtxBuffer[signed_cast(index + v_offset)], texture_size);
						}
					);

					auto state = states;

					//draw with texture
					if (cmd.TextureId)
					{
						const auto texture = cmd.TextureId;
						assert(texture);
						state.texture = texture;
					}

					const auto x = cmd.ClipRect.x - draw_data->DisplayPos.x;
					const auto y = cmd.ClipRect.y - draw_data->DisplayPos.y;
					const auto clip_region = rect_float{ x, y,
						cmd.ClipRect.z - x,
						cmd.ClipRect.w - y };

					target.draw(draw_clamp_region{ vector_drawable{ vertex_array, sf::PrimitiveType::Triangles }, clip_region }, state);
				} //! else no usr callback
			} // !for each cmd
		});
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

	void gui::_image_animation_frame(const resources::animation& a, const vector2 size,
		const resources::animation_frame& f, const sf::Color& tint_colour,
		const sf::Color& border_colour)
	{
		_active_assert();
		const auto texture = resources::animation_functions::get_texture(a);
		
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		const auto flip_x = f.scale_w < 0 || f.w < 0;
		const auto flip_y = f.scale_h < 0 || f.h < 0;

		// calculate the extra size needed to store the image at its offset
		const auto bounds = resources::animation_functions::get_bounding_area(a, size);
		const auto im_min_off = ImVec2{ abs(bounds.x), abs(bounds.y) };
		const auto im_off = ImVec2{ f.off_x, f.off_y };
		const auto im_size = ImVec2{ (size.x + f.off_x) * abs(f.scale_w), (size.y + f.off_y) * abs(f.scale_h) };
		ImRect image_bb(window->DC.CursorPos + im_min_off + im_off, window->DC.CursorPos + im_min_off + im_size);
		const auto bounds_size = ImVec2{ bounds.width, bounds.height };
		ImRect bb(window->DC.CursorPos, window->DC.CursorPos + bounds_size + im_min_off);
		
		const auto border_col = to_imvec4(border_colour);
		if (border_col.w > 0.0f)
		{
			image_bb.Max += ImVec2{ 2, 2 };
			bb.Max += ImVec2{ 2, 2 };
		}

		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, 0))
			return;

		const auto [tex_width, tex_height] = resources::texture_functions::get_size(*texture);

		auto uv0 = ImVec2{ f.x / tex_width, f.y / tex_height };
		auto uv1 = ImVec2{ (f.x + abs(f.w)) / tex_width, (f.y + abs(f.h)) / tex_height };

		if (flip_x)
			std::swap(uv0.x, uv1.x);
		if (flip_y)
			std::swap(uv0.y, uv1.y);

		const auto tint_col = to_imvec4(tint_colour);

		if (border_col.w > 0.0f)
		{
			window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), 0.0f);
			window->DrawList->AddImage(&resources::texture_functions::get_sf_texture(texture), image_bb.Min + ImVec2(1, 1), image_bb.Max - ImVec2(1, 1), uv0, uv1, ImGui::GetColorU32(tint_col));
		}
		else
			window->DrawList->AddImage(&resources::texture_functions::get_sf_texture(texture), image_bb.Min, image_bb.Max, uv0, uv1, ImGui::GetColorU32(tint_col));

		return;
	}

	gui::font *gui::_get_font(const resources::font *f)
	{
		assert(f);

		if (const auto font = _fonts.find(f); font != std::end(_fonts))
			return font->second;

		throw gui_font_missing{ "Font hasn't been created yet" };
	}

	void gui::_create_font(const resources::font *f)
	{
		auto &f_atlas = *_font_atlas;
		ImFontConfig cfg;
		cfg.FontDataOwnedByAtlas = false;
		const auto size = f->source_buffer.size();
		const auto int_size = integer_cast<int>(size);
		//const cast, because f_atlas demands control of the ptr
		//though it won't actually do anything, since FontDataOwned is set to false
		auto font = f_atlas.AddFontFromMemoryTTF(const_cast<std::byte*>(f->source_buffer.data()), int_size, 13.f, &cfg);
		_generate_atlas();
		_fonts.emplace(f, font);
		return;
	}

	void gui::_create_default_font()
	{
		_font_atlas->AddFontDefault();
		_generate_atlas();
		return;
	}

	void gui::_generate_atlas()
	{
		namespace tex = resources::texture_functions;
		//get texture
		auto& d = data::detail::get_data_manager();

		static unique_id font_texture_id = d.get_uid("gui-font-texture-atlas");
		auto t = tex::find_create_texture(d, font_texture_id);

		//get the data and set the correct ids
		int width = 0, height = 0;
		unsigned char* texture_data = nullptr;
		_font_atlas->GetTexDataAsRGBA32(&texture_data, &width, &height);

		//make the texture
		auto& texture = tex::get_sf_texture(t);

		auto sb = std::stringbuf{};
		const auto prev = sf::err().rdbuf(&sb);
		if (!texture.create({ unsigned_cast(width), unsigned_cast(height) }))
		{
			log_error("Unable to create gui texture atlas.");
			log_error(sb.str());
		}
		else
		{
			_font_atlas->SetTexID(&texture);
			texture.update(texture_data);
			//apply correct settings
			tex::set_settings(t,
				{ integer_cast<texture_size_t>(width), integer_cast<texture_size_t>(height) },
				false, //smooth
				false, //repeat 
				false, //mips
				true); //set loaded
		}

        sf::err().rdbuf(prev);
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
		_data->InsertChars(pos, text);
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

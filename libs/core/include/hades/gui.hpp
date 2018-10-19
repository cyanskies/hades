#ifndef HADES_GUI_HPP
#define HADES_GUI_HPP

#include <memory>
#include <string_view>
#include <unordered_map>

#include "imgui.h"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Window/Event.hpp"

#include "hades/timers.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	namespace resources
	{
		struct animation;
		struct font;
	}

	class gui : public sf::Drawable
	{
	public:
		gui();

		gui(const gui&) = delete;
		gui(gui&&) = default;

		gui &operator=(const gui&) = delete;
		gui &operator=(gui&&) = default;

		void activate_context();

		void set_display_size(vector_t<float> size);
		
		//input must be inserted before frame_begin
		bool handle_event(const sf::Event &e);
		void update(time_duration dt);

		void frame_begin();
		void frame_end();

		void show_demo_window();

		enum class window_flags : ImGuiWindowFlags {
			none = ImGuiWindowFlags_::ImGuiWindowFlags_None
		};

		//windows and child windows
		bool window_begin(std::string_view name, bool &closed, window_flags = window_flags::none);
		bool window_begin(std::string_view name, window_flags = window_flags::none);
		void window_end();

		using vector = vector_t<float>;
		bool child_window_begin(std::string_view name, vector size = { 0.f ,0.f }, bool border = false, window_flags = window_flags::none);
		void child_window_end();

		//window utilities
		vector window_position() const;
		vector window_size() const;
		void next_window_position(vector);
		void next_window_size(vector);

		//shared parameters
		void push_font(const resources::font*);
		void pop_font();

		//layouts
		void seperator_horizontal();
		void layout_horizontal(float pos = 0.f, float width = -1.f);
		void layout_vertical();
		void vertical_spacing();

		//text widgets
		void text(std::string_view);
		void text_coloured(std::string_view, sf::Color);
		void text_disabled(std::string_view);

		//text_label
		void text_bullet(std::string_view);

		//widgets
		void image(const resources::animation&, const vector &size, time_point time = time_point{}, const sf::Color &tint_colour = sf::Color::White, const sf::Color &border_colour = sf::Color::Transparent);
		bool image_button(const resources::animation&, const vector &size, time_point time = time_point{}, const sf::Color &background_colour = sf::Color::Transparent, const sf::Color &tint_colour = sf::Color::White);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		static constexpr std::string_view version();

	private:
		using font = ImFont;

		void _activate_context();
		void _active_assert() const;
		static font* _get_font(const resources::font*);
		static font* _create_font(const resources::font*);
		static void _generate_atlas();

		using context = ImGuiContext;
		using context_ptr = std::unique_ptr<context, decltype(&ImGui::DestroyContext)>;
		context_ptr _my_context{nullptr, ImGui::DestroyContext};

		//static font atlas will be shared for all instances of gui
		static std::unique_ptr<ImFontAtlas> _font_atlas;
		static std::unordered_map<const resources::font*, ImFont*> _fonts;
	};
}

#endif //!HADES_GUI_HPP

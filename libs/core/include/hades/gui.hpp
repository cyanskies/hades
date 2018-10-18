#ifndef HADES_GUI_HPP
#define HADES_GUI_HPP

#include <memory>
#include <string_view>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "hades/font.hpp"
#include "hades/texture.hpp"
#include "hades/timers.hpp"
#include "hades/vector_math.hpp"

//fwd declarations of ImGui types and functions
struct ImGuiContext;
struct ImFontAtlas;
struct ImFont;

namespace ImGui
{
	ImGuiContext *CreateContext(ImFontAtlas*);
	void DestroyContext(ImGuiContext*);
}

namespace hades
{
	class gui_exception : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	//thrown if the gui object doesn't have a context
	class gui_no_context final : public gui_exception
	{
	public:
		using gui_exception::gui_exception;
	};

	namespace resources
	{
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
		using font = ImFont;
		const font *add_font(const resources::font*);

		//input must be inserted before frame_begin
		void input_actions(time_duration dt);

		void frame_begin();
		void frame_end();

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		void show_demo_window();

		static constexpr std::string_view version();

		//window_begin
		//window_end

		//child_begin
		//child_end

	private:
		void _activate_context();
		void _active_assert() const;
		void _generate_atlas();

		using context = ImGuiContext;
		using context_ptr = std::unique_ptr<context, decltype(&ImGui::DestroyContext)>;
		context_ptr _my_context{ImGui::CreateContext(nullptr), ImGui::DestroyContext};

		//each gui instance will create a texture, 
		//should we add a destroy function to data_manager so these can be cleaned up?
		unique_id _font_atlas_id;
	};
}

#endif //!HADES_GUI_HPP

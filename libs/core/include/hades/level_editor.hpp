#ifndef HADES_LEVEL_EDITOR_HPP
#define HADES_LEVEL_EDITOR_HPP

#include <string_view>
#include <tuple>

#include "hades/gui.hpp"
#include "hades/level.hpp"
#include "hades/properties.hpp"
#include "hades/state.hpp"
#include "hades/types.hpp"

namespace hades::detail
{
	class level_editor_impl : public state
	{
	public:
		void init() override;
		bool handle_event(const event&) override;
		void reinit() override;

	protected:
		virtual void _hand_component_setup() = 0;

		gui _gui;
		float _window_width = 0.f, _window_height = 0.f;
		console::property_int _toolbox_width;
		level_size_t _level_x = 0, _level_y = 0;
		std::size_t _active_brush = std::numeric_limits<std::size_t>::max();

		level *_level = nullptr;
	};
}

namespace hades
{
	void create_editor_console_variables();

	template<typename ...Components>
	class basic_level_editor final : public detail::level_editor_impl
	{
	public:
		void update(time_duration delta_time, const sf::RenderTarget&, input_system::action_set) override;
		void draw(sf::RenderTarget &target, time_duration delta_time) override;
		
	private:
		void _hand_component_setup() override;

		using component_tuple = std::tuple<Components...>;
		component_tuple _editor_components;
	};

	using level_editor = basic_level_editor<>;
}

namespace hades::cvars
{
	using namespace std::string_view_literals;
	constexpr auto editor_toolbox_width = "editor_toolbox_width"sv; // set to -1 for auto 1/3 screen width
}

namespace hades::cvars::default_value
{
	constexpr int32 editor_toolbox_width = -1;
}

#include "hades/detail/level_editor.inl"

#endif //!HADES_LEVEL_EDITOR_HPP

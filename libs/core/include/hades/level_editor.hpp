#ifndef HADES_LEVEL_EDITOR_HPP
#define HADES_LEVEL_EDITOR_HPP

#include <string_view>
#include <tuple>

#include "hades/gui.hpp"
#include "hades/state.hpp"
#include "hades/types.hpp"

namespace hades::detail
{
	class level_editor_impl : public state
	{
	public:
		bool handle_event(const event&) override;
		void reinit() override;

	protected:
		gui _gui;
		float _window_width = 0.f, _window_height = 0.f;
		int32 _level_x = 0, _level_y = 0;
		std::size_t _active_brush;
	};
}

namespace hades
{
	void create_editor_console_varaibles();

	template<typename ...Components>
	class basic_level_editor : public detail::level_editor_impl
	{
	public:
		void init() override;
		void update(time_duration delta_time, const sf::RenderTarget&, input_system::action_set) override;
		void draw(sf::RenderTarget &target, time_duration delta_time) override;
		
	private:
		using component_tuple = std::tuple<Components...>;
		component_tuple _editor_components;
	};

	using level_editor = basic_level_editor<>;
}

namespace hades::cvars
{

}

namespace hades::cvars::default_value
{

}

#include "hades/detail/level_editor.inl"

#endif //!HADES_LEVEL_EDITOR_HPP

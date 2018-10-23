#ifndef HADES_LEVEL_EDITOR_HPP
#define HADES_LEVEL_EDITOR_HPP

#include <string_view>
#include <tuple>

#include "hades/gui.hpp"
#include "hades/state.hpp"

namespace hades::detail
{
	class level_editor_impl : public state
	{
	public:
		bool handle_event(const event&) override;
		void reinit() override;

	protected:
		gui _gui;
		float _width = 0.f, _height = 0.f;
	};
}

namespace hades
{
	template<typename ...Components>
	class basic_level_editor : public detail::level_editor_impl
	{
	public:
		void update(time_duration delta_time, const sf::RenderTarget&, input_system::action_set) override;
		void draw(sf::RenderTarget &target, time_duration delta_time) override;
		
	private:
		using component_tuple = std::tuple<Components...>;
		component_tuple _editor_components;
	};

	using level_editor = basic_level_editor<>;
}

#include "hades/detail/level_editor.inl"

#endif //!HADES_LEVEL_EDITOR_HPP

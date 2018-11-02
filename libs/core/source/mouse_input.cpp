#include "hades/mouse_input.hpp"

#include <string_view>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/View.hpp"

#include "hades/data.hpp"

namespace hades
{
	namespace input
	{
		input_id mouse_position = input_id::zero;
		input_id mouse_left = input_id::zero;
	}

	namespace pointer
	{
		vector_f to_world_coords(const sf::RenderTarget &t, vector_i pos, const sf::View &v)
		{
			const auto r = t.mapPixelToCoords({ pos.x, pos.y }, v);
			return { r.x, r.y };
		}

		vector_i from_world_coords(const sf::RenderTarget &t, vector_f pos, const sf::View &v)
		{
			const auto r = t.mapCoordsToPixel({ pos.x, pos.y }, v);
			return { r.x, r.y };
		}

		vector_i snap_coords_to_grid(vector_i coord, types::int32 grid_size)
		{
			const auto coordf = vector_f{ static_cast<vector_f::value_type>(coord.x),
										static_cast<vector_f::value_type>(coord.y) };

			const auto snap_pos = coordf -
				vector_f{ static_cast<vector_f::value_type>(std::abs(std::fmod(coordf.x, grid_size))),
				static_cast<vector_f::value_type>(std::abs((std::fmod(coordf.y, grid_size)))) };

			return { static_cast<vector_i::value_type>(snap_pos.x),
				static_cast<vector_i::value_type>(snap_pos.y) };
		}
	}

	void register_mouse_input(input_system &bind)
	{
		using namespace std::string_view_literals;
		constexpr auto m = "mouse"sv;
		constexpr auto m1 = "mouseleft"sv;

		//collect unique names for actions
		input::mouse_position = data::make_uid(m);
		input::mouse_left = data::make_uid(m1);

		//bind them
		bind.create(input::mouse_position, false, m);
		bind.create(input::mouse_left, false, m1);
	}
}
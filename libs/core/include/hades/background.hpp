#ifndef HADES_BACKGROUND_HPP
#define HADES_BACKGROUND_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/colour.hpp"
#include "hades/level.hpp"
#include "hades/math.hpp"
#include "hades/resource_base.hpp"
#include "hades/tiled_sprite.hpp"

namespace hades::resources
{
	struct background_t {};

	struct background : resource_type<background_t>
	{
		struct layer
		{
            resource_link<animation> anim;
			vector2_float offset{ 0.f, 0.f };
			vector2_float parallax{ 0.f, 0.f };
		};

		void load(data::data_manager&) final override;

        colour fill_colour = colours::black;
		std::vector<layer> layers;
	};
}

namespace hades
{
	void register_background_resource(data::data_manager&);

	class background : public sf::Drawable
	{
	public:
		using layer = level::background_layer;

		struct background_layer
		{
			const resources::animation *animation = nullptr;
			vector2_float offset{ 0.f, 0.f };
			vector2_float parallax{ 0.f, 0.f };
			tiled_sprite sprite;
		};

		background() = default;
		background(const resources::background&);
		background(vector2_float size, const std::vector<layer>& = std::vector<layer>{}, colour = colours::black);
		
		background(const background&) = default;
		background &operator=(const background&) = default;

		void set_colour(colour) noexcept;
		void set_size(vector2_float) noexcept;
		// throws: data::resource_error if the animation cannot be found
		void add(layer);
		void clear() noexcept;

		//must be called at least once to generate the mesh
		void update(time_point = time_point{});
		//should be called once per frame
		void update(rect_float view) noexcept;
		// throws: bad_alloc
		void update(time_point, rect_float);
		void draw(sf::RenderTarget&, const sf::RenderStates&) const override;

	private:
		sf::RectangleShape _backdrop;
		std::vector<background_layer> _layers;
		vector2_float _size{};
	};
}

#endif //!HADES_BACKGROUND_HPP

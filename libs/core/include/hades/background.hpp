#ifndef HADES_BACKGROUND_HPP
#define HADES_BACKGROUND_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/colour.hpp"
#include "hades/level.hpp"
#include "hades/tiled_sprite.hpp"

namespace hades
{
	class background : public sf::Drawable
	{
	public:
		using layer = level::background_layer;

		struct background_layer
		{
			const resources::animation *animation = nullptr;
			vector_float offset{ 0.f, 0.f };
			vector_float parallax{ 0.f, 0.f };
			tiled_sprite sprite;
		};

		background() = default;
		background(vector_float size, const std::vector<layer>& = std::vector<layer>{}, colour = colours::black);
		
		background(const background&) = default;
		background &operator=(const background&) = default;

		~background() noexcept = default;

		void set_colour(colour);
		void set_size(vector_float);
		void add(layer);
		void clear() noexcept;

		//must be called at least once to generate the mesh
		void update(time_point = time_point{});
		//should be called once per frame
		void update(vector_float view_position);
		void draw(sf::RenderTarget&, sf::RenderStates) const override;

	private:
		sf::RectangleShape _backdrop;
		std::vector<background_layer> _layers;
		vector_float _size;
	};
}

#endif //!HADES_BACKGROUND_HPP
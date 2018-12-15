#ifndef HADES_DRAW_CLAMP_HPP
#define HADES_DRAW_CLAMP_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "SFML/Graphics/RenderStates.hpp"

#include "hades/math.hpp"

namespace hades
{
	//clamps the drawable to a  world region
	//doesn't support view rotation or custom viewports
	class draw_clamp_region : public sf::Drawable
	{
	public:
		draw_clamp_region(const sf::Drawable &d, const rect_float &rect) noexcept
			: _drawable{ d }, _region{rect}
		{}

		draw_clamp_region(const sf::Drawable &d, const sf::FloatRect &rect) noexcept
			: _drawable{ d }, _region{ rect_float{rect.left, rect.top, rect.width, rect.height} }
		{}

		~draw_clamp_region() noexcept = default;

		draw_clamp_region(const draw_clamp_region&) = delete;
		draw_clamp_region(draw_clamp_region&&) = delete;
		draw_clamp_region &operator=(const draw_clamp_region&) = delete;
		draw_clamp_region &operator=(draw_clamp_region&&) = delete;

		void draw(sf::RenderTarget&, sf::RenderStates) const override;

	private:
		const sf::Drawable &_drawable;
		const rect_float &_region;
	};

	//clamps drawing the target to a window region
	//this is the same as GLScissor
	class draw_clamp_window : public sf::Drawable
	{
	public:
		draw_clamp_window(const sf::Drawable &d, const rect_int &rect) noexcept
			: _drawable{ d }, _region{ rect }
		{}

		draw_clamp_window(const sf::Drawable &d, const sf::IntRect &rect) noexcept
			: _drawable{ d }, _region{ rect_int{rect.left, rect.top, rect.width, rect.height} }
		{}

		~draw_clamp_window() noexcept = default;

		draw_clamp_window(const draw_clamp_region&) = delete;
		draw_clamp_window(draw_clamp_region&&) = delete;
		draw_clamp_window &operator=(const draw_clamp_region&) = delete;
		draw_clamp_window &operator=(draw_clamp_region&&) = delete;

		void draw(sf::RenderTarget&, sf::RenderStates) const;

	private:
		const sf::Drawable &_drawable;
		const rect_int &_region;
	};
}

#endif //!HADES_DRAW_CLAMP_HPP

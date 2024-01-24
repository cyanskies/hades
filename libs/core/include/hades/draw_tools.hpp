#ifndef HADES_DRAW_TOOLS_HPP
#define HADES_DRAW_TOOLS_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/OpenGL.hpp"

#include "hades/rectangle_math.hpp"

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

		void draw(sf::RenderTarget&, const sf::RenderStates&) const override;

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

        [[deprecated]] // this constructor is a bug
		draw_clamp_window(const sf::Drawable &d, const sf::IntRect &rect) noexcept
			: _drawable{ d }, _region{ rect_int{rect.left, rect.top, rect.width, rect.height} }
		{}

		~draw_clamp_window() noexcept = default;

		draw_clamp_window(const draw_clamp_region&) = delete;
		draw_clamp_window(draw_clamp_region&&) = delete;
		draw_clamp_window &operator=(const draw_clamp_region&) = delete;
		draw_clamp_window &operator=(draw_clamp_region&&) = delete;

		void draw(sf::RenderTarget&, const sf::RenderStates&) const override;

	private:
		const sf::Drawable &_drawable;
		const rect_int &_region;
	};

	namespace depth_buffer
	{
		inline void setup() noexcept
		{
			glDepthFunc(GL_LEQUAL);
			glDepthRange(1.0f, 0.0f);
			glAlphaFunc(GL_GREATER, 0.0f);
			return;
		}

		inline void enable() noexcept
		{
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_ALPHA_TEST);
			glDepthMask(GL_TRUE);
			return;
		}

		inline void disable() noexcept
		{
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_ALPHA_TEST);
			glDepthMask(GL_FALSE);
			return;
		}

		inline void clear() noexcept
		{
			glClearDepth(1.0f);
			glClear(GL_DEPTH_BUFFER_BIT);
			return;
		}	
	}
}

#endif //!HADES_DRAW_CLAMP_HPP

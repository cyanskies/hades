#include "hades/draw_clamp.hpp"

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/RenderTarget.hpp"

#include "SFML/OpenGL.hpp"

//these implementations are in private to avoid bringing GL globals into everything

namespace hades
{
	void draw_clamp_region::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		const auto &view = t.getView();
		//we can't clamp for rotated view
		assert(view.getRotation() == 0.f);

		//move the region to account for the views position
		const auto view_size = view.getSize();
		const auto view_pos = view.getCenter() - view_size / 2.f;
		const auto window_rect_pos = vector_float{ _region.x - view_pos.x, _region.y - view_pos.y };

		// TODO: stretch or shrink the region to account for view_size and view_port
		/*
		const auto &view_port = t.getViewport(view);
		const auto &view_size = view.getSize();
		const auto &window_rect_size = 0;
		*/

		//we don't support custom viewport settings
		assert((view.getViewport() == sf::FloatRect{ {0.f, 0.f}, {1.f, 1.f} }));

		glEnable(GL_SCISSOR_TEST);
		glScissor(static_cast<GLint>(window_rect_pos.x),
			static_cast<GLint>(view_size.y - window_rect_pos.y - _region.height),
			static_cast<GLsizei>(_region.width),
			static_cast<GLsizei>(_region.height));

		t.draw(_drawable, s);

		glDisable(GL_SCISSOR_TEST);
	}

	void draw_clamp_window::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(static_cast<GLint>(_region.x),
			static_cast<GLint>(_region.y),
			static_cast<GLsizei>(_region.width),
			static_cast<GLsizei>(_region.height));

		t.draw(_drawable, s);

		glDisable(GL_SCISSOR_TEST);
	}
}

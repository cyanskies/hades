#ifndef HADES_TILED_SPRITE_HPP
#define HADES_TILED_SPRITE_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Transformable.hpp"

#include "hades/timers.hpp"
#include "hades/vector_math.hpp"
#include "hades/vertex_buffer.hpp"

namespace hades::resources
{
	struct animation;
}

namespace hades
{
	class tiled_sprite : public sf::Drawable, public sf::Transformable
	{
	public:
		void set_animation(const resources::animation*, time_point);
		void set_size(vector_float);

		void draw(sf::RenderTarget&, sf::RenderStates) const override;

	private:
		void _generate_buffer();

		const resources::animation* _animation{ nullptr };
		vertex_buffer _vert_buffer{ sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Dynamic };
		vector_float _frame{ -1.f,-1.f };
		vector_float _size;
	};
}

#endif //!HADES_TILED_SPRITE_HPP

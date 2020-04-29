#ifndef HADES_TILED_SPRITE_HPP
#define HADES_TILED_SPRITE_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Transformable.hpp"

#include "hades/timers.hpp"
#include "hades/vector_math.hpp"
#include "hades/vertex_buffer.hpp"

//a animated sprite with tiling

namespace hades::resources
{
	struct animation;
}

namespace hades
{
	class tiled_sprite : public sf::Drawable, public sf::Transformable
	{
	public:
		struct dont_regen_t {};

		//pass tiled_sprite::dont_regen to defer regenerating the vertex array
		void set_animation(const resources::animation*, time_point);
		void set_animation(const resources::animation*, time_point, dont_regen_t);

		void set_size(vector_float);
		void set_size(vector_float, dont_regen_t) noexcept;

		void draw(sf::RenderTarget&, sf::RenderStates) const override;

		static constexpr dont_regen_t dont_regen{};

	private:
		void _generate_buffer();

		const resources::animation* _animation{ nullptr };
		vertex_buffer _vert_buffer{ sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Dynamic };
		vector_float _frame{ -1.f,-1.f };
		vector_float _size{};
	};
}

#endif //!HADES_TILED_SPRITE_HPP

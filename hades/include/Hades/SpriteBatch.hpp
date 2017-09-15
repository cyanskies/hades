#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Shader.hpp"
#include "SFML/Graphics/Sprite.hpp"

#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"
#include "Hades/type_erasure.hpp"

//A thread safe collection of animated sprites
//the object manages batching draw calls to render more efficiently
//the object also manages animation times.

namespace hades {
	struct ShaderUniformBase
	{
		virtual ~ShaderUniformBase() {}
		virtual void apply(const types::string&, sf::Shader&) const = 0;
		const bool array;
	};

	template<typename T>
	struct ShaderUniform final
	{
		ShaderUniform() : array(false) {}

		virtual void apply(const types::string& n, sf::Shader &s) const override
		{
			s.setUniform(n, uniform);
		}

		T uniform;
	};

	template<typename T>
	struct ShaderUniformArray final
	{
		ShaderUniformArray() : array(true) {}

		virtual void apply(const types::string& n, sf::Shader &s) const override
		{
			s.setUniformArray(n, uniform.data(), uniform.size());
		}

		std::vector<T> uniform;
	};

	struct Sprite
	{
		types::int16 layer;
		sf::Sprite sprite;
		property_bag<types::string> uniforms;
	};

	struct Sprite_p
	{
		Sprite* s = nullptr;
	};

	bool operator<(const Sprite_p &lhs, const Sprite_p &rhs)
	{
		return lhs.s->layer < rhs.s->layer;
	}

	class SpriteBatch : public sf::Drawable // drawable
	{
	public:
		using sprite_id = types::uint64;
		sprite_id createSprite(/*animation*/);
		void  destroySprite();

		void setAnimation();
		void setPosition();
		void setLayer();

		void setUniform();

		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	private:
		//sorted by layer for drawing
		std::multiset<Sprite_p> _draw_list;
		std::map<sprite_id, std::unique_ptr<Sprite>> _sprites;
	};
}

#endif //HADES_SPRITE_BATCH_HPP

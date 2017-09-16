#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include <mutex>

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
	//NOTE: this current impl does have any performance benifits over using sprites
	// the purpose here is to delop the API for the SpriteBatch
	class SpriteBatch : public sf::Drawable
	{
	public:
		using sprite_id = types::uint64;

		//Begin thread safe methods
		sprite_id createSprite(/*animation*/);
		void destroySprite(sprite_id id);

		void changeAnimation(sprite_id id /*animation*/);
		void setAnimationProgress(sprite_id id);

		void setPosition(sprite_id id, sf::Vector2f pos);
		void setLayer(sprite_id id, types::int16 l);

		template<typename T>
		void setUniform(sprite_id id, types::string name, T value);

		template<typename T>
		void setUniform(sprite_id id, types::string name, std::vector<T> value);
		//End thread safe methods

		//sorts the draw list by layer, to ensure layer correct rendering
		void sort_by_layer();
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates()) const override;

	private:
		struct ShaderUniformBase
		{
			virtual ~ShaderUniformBase() {}
			virtual void apply(const types::string&, sf::Shader&) const = 0;
		};

		template<typename T>
		struct ShaderUniform final
		{
			virtual void apply(const types::string& n, sf::Shader &s) const override
			{
				s.setUniform(n, uniform);
			}

			T uniform;
		};

		template<typename T>
		struct ShaderUniformArray final
		{
			virtual void apply(const types::string& n, sf::Shader &s) const override
			{
				s.setUniformArray(n, uniform.data(), uniform.size());
			}

			std::vector<T> uniform;
		};

		struct Sprite
		{
			std::mutex mut;
			types::int16 layer;
			sf::Sprite sprite;
			property_bag<types::string> uniforms;
		};

		mutable std::mutex _collectionMutex; //mutex to ensure two threads don't try to add/search/erase from the two collections at the same time
		//sorted by layer for drawing
		std::vector<Sprite*> _draw_list;
		std::map<sprite_id, std::unique_ptr<Sprite>> _sprites;
		sprite_id _id_count = std::numeric_limits<sprite_id>::min();
	};
}

#endif //HADES_SPRITE_BATCH_HPP

#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include <mutex>
#include <shared_mutex>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Shader.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/System/Time.hpp"

#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

//A thread safe collection of animated sprites
//the object manages batching draw calls to render more efficiently
//the object also manages animation times.

namespace hades {
	class InvalidSpriteId : public std::logic_error
	{
	public:
		using std::logic_error::logic_error;
	};

	namespace sprite_utility
	{
		//throws spritebatch exception or something if trying to modify a missing sprite
		template<class K, class V>
		typename std::map<K, V>::iterator GetElement(std::map<K, V> &m, K id)
		{
			auto it = m.find(id);
			if (it == std::end(m))
				throw InvalidSpriteId("Tried to retreive missing Id, Id was: " + std::to_string(id));

			return it;
		}

		using type_id = types::uint32;

		struct ShaderUniformBase
		{
			virtual ~ShaderUniformBase() {}
			virtual void apply(const types::string&, sf::Shader&) const = 0;
			virtual bool operator==(const ShaderUniformBase& other) const = 0;

			virtual type_id getType() const = 0;
		};

		bool operator!=(const ShaderUniformBase &lhs, const ShaderUniformBase &rhs)
		{
			return !(lhs == rhs);
		}

		template<typename T>
		struct ShaderUniform final
		{
			virtual void apply(const types::string& n, sf::Shader &s) const override
			{
				static_assert(resources::shader::valid_type<T>(), "Passed type cannot be used as a SFML Uniform");
				s.setUniform(n, uniform);
			}

			virtual bool operator==(const ShaderUniformBase& other) const
			{
				if (other.getType() != type)
					return false;

				ShaderUniform<T> &ref = static_cast<ShaderUniform<T>&>(other);

				return ref.uniform == uniform;
			}

			virtual type_id getType() const
			{
				return type;
			}

			T uniform;
			static const type_id type;
		};

		template<typename T>
		type_id ShaderUniform<T>::type = type_count++;

		template<typename T>
		struct ShaderUniformArray final
		{
			virtual void apply(const types::string& n, sf::Shader &s) const override
			{
				static_assert(resources::shader::valid_array_type<T>(), "Passed array type cannot be used as a SFML Uniform");
				s.setUniformArray(n, uniform.data(), uniform.size());
			}

			virtual bool operator==(const ShaderUniformBase& other) const
			{
				if (other.getType() != type)
					return false;

				ShaderUniformArray<T> &ref = static_cast<ShaderUniformArray<T>&>(other);

				return ref.uniform == uniform;
			}

			virtual type_id getType() const
			{
				return type;
			}

			std::vector<T> uniform;
			static const type_id type;
		};

		template<typename T>
		type_id ShaderUniformArray<T>::type = type_count++;

		extern types::uint32 type_count;

		using layer_t = types::int16;

		using uniform_map = std::map<types::string, std::unique_ptr<sprite_utility::ShaderUniformBase>>;

		struct Sprite
		{
			std::mutex mut;
			layer_t layer;
			sf::Sprite sprite;
			const resources::animation *animation;
			uniform_map uniforms;
		};
	}

	//NOTE: this current impl does have any performance benifits over using sprites
	// the purpose here is to delop the API for the SpriteBatch
	class SpriteBatch : public sf::Drawable
	{
	public:
		using sprite_id = types::uint64;
		//clears all of the stored uniforms
		void cleanUniforms();

		//Begin thread safe methods
		sprite_id createSprite(sf::Vector2f position, sprite_utility::layer_t l, const resources::animation *a, sf::Time t);
		void destroySprite(sprite_id id);

		bool exists(sprite_id id) const;

		void changeAnimation(sprite_id id, const resources::animation *a, sf::Time t);

		void setPosition(sprite_id id, sf::Vector2f pos);
		void setLayer(sprite_id id, sprite_utility::layer_t l);

		template<typename T>
		void setUniform(sprite_id id, types::string name, T value);

		template<typename T>
		void setUniform(sprite_id id, types::string name, std::vector<T> value);
		//End thread safe methods

		//generates the fewest number of vertex array needed to draw all the contained sprites
		//usually called once before draw
		void prepare();
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates()) const override;

	private:
		//for the vertex batch method, store all sprite details(anim, anim progress)
		//and push them into a vetex buffer each frame that most efficiently reduces draw calls.

		mutable std::shared_mutex _collectionMutex; //mutex to ensure two threads don't try to add/search/erase from the two collections at the same time
		//sorted by layer for drawing
		std::vector<sprite_utility::Sprite*> _draw_list;
		std::map<sprite_id, std::unique_ptr<sprite_utility::Sprite>> _sprites;
		sprite_id _id_count = std::numeric_limits<sprite_id>::min();
	};

	template<typename T>
	void SpriteBatch::setUniform(typename SpriteBatch::sprite_id id, types::string name, T value)
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		auto it = sprite_utility::GetElement(_sprites, id);

		{
			std::lock_guard<std::mutex> slk(it->second->mut);
			auto u = std::make_unique<sprite_utility::ShaderUniform<T>>();
			u->uniform = value;
			it->second->uniforms.insert({ name, std::move(u) });
		}
	}

	template<typename T>
	void SpriteBatch::setUniform(typename SpriteBatch::sprite_id id, types::string name, std::vector<T> value)
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		auto it = sprite_utility::GetElement(_sprites, id);

		{
			std::lock_guard<std::mutex> slk(it->second->mut);
			auto u = std::make_unique<sprite_utility::ShaderUniformArray<T>>();
			u->uniform = value;
			it->second->uniforms.insert({ name, std::move(u) });
		}
	}
}

#endif //HADES_SPRITE_BATCH_HPP

#include "Hades/SpriteBatch.hpp"

#include "Hades/Animation.hpp"

namespace hades
{
	using type_id = types::uint32;

	struct ShaderUniformBase
	{
		virtual ~ShaderUniformBase() {}
		virtual void apply(const types::string&, sf::Shader&) const = 0;
		virtual bool operator==(const ShaderUniformBase& other) const = 0;

		virtual type_id getType() const = 0;
	};

	namespace
	{
		bool operator!=(const ShaderUniformBase &lhs, const ShaderUniformBase &rhs)
		{
			return !(lhs == rhs);
		}

		template<typename T>
		struct ShaderUniform final
		{
			virtual void apply(const types::string& n, sf::Shader &s) const override
			{
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
			static type_id type;
		};

		template<typename T>
		type_id ShaderUniform<T>::type = type_count++;

		template<typename T>
		struct ShaderUniformArray final
		{
			virtual void apply(const types::string& n, sf::Shader &s) const override
			{
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
			static type_id type;
		};

		template<typename T>
		type_id ShaderUniformArray<T>::type = type_count++;

		types::uint32 type_count = 0;

		using uniform_map = SpriteBatch::uniform_map;

		//compare if two sets of uniforms are identical
		bool ShaderUniformsEqual(const uniform_map &lhs, const uniform_map &rhs)
		{
			if (lhs.size() != rhs.size())
				return false;

			auto lhs_it = lhs.begin(), rhs_it = rhs.begin();

			while (lhs_it != lhs.end())
			{
				if (lhs_it->first != rhs_it->first)
					return false;

				const ShaderUniformBase *lhs_base, *rhs_base;
				lhs_base = &*(lhs_it->second);
				rhs_base = &*(rhs_it->second);

				if (*lhs_base != *rhs_base)
					return false;

				lhs_it++; rhs_it++;
			}

			return true;
		}

		//apply all the uniforms to the shader
		//TODO: error checking/handling
		void ApplyUniforms(const uniform_map &uniforms, sf::Shader &shader)
		{
			auto uniforms_it = uniforms.begin();
			while (uniforms_it != uniforms.end())
			{
				ShaderUniformBase *uniform_base;
				uniform_base = &*(uniforms_it->second);
				uniform_base->apply(uniforms_it->first, shader);
				uniforms_it++;
			}
		}
	}

	//NOTE: STORE unique ptrs to ShaderUniformBase in propertyBag
	typename SpriteBatch::sprite_id SpriteBatch::createSprite(sf::Vector2f position, layer_t l, const resources::animation *a, sf::Time t)
	{	
		auto s = std::make_unique<Sprite>();
		s->layer = l;
		s->animation = a;
		s->sprite = sf::Sprite(a->tex->value);
		s->sprite.setPosition(position);
		apply_animation(a, t, s->sprite);

		auto id = _id_count++;
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);
		_sprites.emplace(id, std::move(s));

		return id;
	}

	void  SpriteBatch::destroySprite(typename SpriteBatch::sprite_id id)
	{
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);
		auto it = _sprites.find(id);
		if (it != std::end(_sprites))
		{
			_sprites.erase(it);
		}
	}

	bool SpriteBatch::exists(typename SpriteBatch::sprite_id id) const
	{
		return _sprites.find(id) != std::end(_sprites);
	}

	void SpriteBatch::changeAnimation(typename SpriteBatch::sprite_id id, const resources::animation *a, sf::Time t)
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		auto it = _sprites.find(id);
		if (it == std::end(_sprites))
			return; //TODO: throw error

		{
			std::lock_guard<std::mutex> slk(it->second->mut);
			it->second->animation = a;
			apply_animation(a, t, it->second->sprite);
		}
	}
}

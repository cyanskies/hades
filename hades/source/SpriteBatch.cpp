#include "Hades/SpriteBatch.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

#include "Hades/Animation.hpp"

namespace hades
{
	namespace sprite_utility
	{
		types::uint32 type_count = 0;
	}

	using uniform_map = sprite_utility::uniform_map;

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

			const sprite_utility::ShaderUniformBase *lhs_base = nullptr, *rhs_base = nullptr;
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
			sprite_utility::ShaderUniformBase *uniform_base = nullptr;
			uniform_base = &*(uniforms_it->second);
			uniform_base->apply(uniforms_it->first, shader);
			uniforms_it++;
		}
	}

	void SpriteBatch::cleanUniforms()
	{
		for (auto &s : _sprites)
			s.second->uniforms.clear();
	}

	//NOTE: STORE unique ptrs to ShaderUniformBase in propertyBag
	typename SpriteBatch::sprite_id SpriteBatch::createSprite(sf::Vector2f position, sprite_utility::layer_t l, const resources::animation *a, sf::Time t)
	{	
		auto s = std::make_unique<sprite_utility::Sprite>();
		s->layer = l;
		s->animation = a;
		s->sprite = sf::Sprite(a->tex->value);
		s->sprite.setPosition(position);
		animation::Apply(a, t, s->sprite);

		auto id = _id_count++;
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);
		_sprites.emplace(id, std::move(s));

		return id;
	}

	void  SpriteBatch::destroySprite(typename SpriteBatch::sprite_id id)
	{
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);
		auto it = sprite_utility::GetElement(_sprites, id);
		_sprites.erase(it);
	}

	bool SpriteBatch::exists(typename SpriteBatch::sprite_id id) const
	{
		return _sprites.find(id) != std::end(_sprites);
	}

	void SpriteBatch::changeAnimation(typename SpriteBatch::sprite_id id, const resources::animation *a, sf::Time t)
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		auto it = sprite_utility::GetElement(_sprites, id);

		{
			std::lock_guard<std::mutex> slk(it->second->mut);
			it->second->animation = a;
			animation::Apply(a, t, it->second->sprite);
		}
	}

	void SpriteBatch::setPosition(typename SpriteBatch::sprite_id id, sf::Vector2f pos)
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		auto it = sprite_utility::GetElement(_sprites, id);

		{
			std::lock_guard<std::mutex> slk(it->second->mut);
			it->second->sprite.setPosition(pos);
		}
	}

	void SpriteBatch::setLayer(typename SpriteBatch::sprite_id id, sprite_utility::layer_t l)
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		auto it = sprite_utility::GetElement(_sprites, id);

		{
			std::lock_guard<std::mutex> slk(it->second->mut);
			it->second->layer = l;
		}
	}

	bool SortSpritePtr(sprite_utility::Sprite *lhs, sprite_utility::Sprite *rhs)
	{
		assert(lhs && rhs);

		//draw lowest layer first
		if (lhs->layer == rhs->layer)
		{
			auto lpos = lhs->sprite.getPosition(),
				rpos = rhs->sprite.getPosition();
			//if the layer is the same, then draw lowest y value first, or lowest x value first
			if (lpos.y == rpos.y)
				return lpos.x < rpos.x;
			else
				return rpos.y < rpos.y;

		}
		else
			return lhs->layer < rhs->layer;
	}

	void SpriteBatch::prepare()
	{
		std::sort(_draw_list.begin(), _draw_list.end(), SortSpritePtr);
	}

	void SpriteBatch::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (auto &s : _draw_list)
		{
			sf::Shader *shader = nullptr;
			if (s->animation->anim_shader && s->animation->anim_shader->loaded)
				shader = &s->animation->anim_shader->value;

			if (shader)
				target.draw(s->sprite, shader);
			else
				target.draw(s->sprite);
		}
	}
}

#include "Hades/SpriteBatch.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

#include "Hades/Animation.hpp"
#include "Hades/exceptions.hpp"
#include "Hades/Types.hpp"

namespace hades
{
	namespace sprite_utility
	{
		bool operator==(const SpriteSettings &lhs, const SpriteSettings &rhs)
		{
			return lhs.layer == rhs.layer
				&& lhs.texture == rhs.texture
				&& lhs.shader == rhs.shader;
		}

		found_sprite FindSprite(std::vector<batch> sbatch, Sprite::sprite_id id)
		{
			for (auto batch_index = 0; batch_index < sbatch.size(); ++batch_index)
			{
				for (auto sprite_index = 0; sprite_index < sbatch[batch_index].second.size(); ++sprite_index)
				{
					if (auto sprite = &sbatch[batch_index].second[sprite_index]; sprite->id == id)
					{
						return { batch_index, sprite_index, sprite };
					}
				}
			}

			throw invalid_argument("Sprite not found in SpriteBatch, id was: " + to_string(id));
		}

		void MoveSprite(std::vector<batch> sbatch, index_type batch_index, index_type sprite_index, const SpriteSettings &settings)
		{
			//get the sprite out of its current batch
			auto *batch = &sbatch[batch_index];
			auto sprite = batch->second[sprite_index];

			//erase the sprite from the old batch
			auto iter = std::begin(batch->second) + sprite_index;
			batch->second.erase(iter);

			batch = nullptr;
			//find a new batch to place it into
			for (auto &b : sbatch)
			{
				if (b.first == settings)
				{
					batch = &b;
					break;
				}
				
				sbatch.push_back({ settings, {} });
				batch = &sbatch.back();
			}

			assert(batch);
			batch->second.push_back(sprite);
		}
	}

	void SpriteBatch::clear()
	{
		//get the exclusive lock
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);

		//clear all the vectors
		//NOTE: we don't clear the base vectors, we want to keep the reserved 
		//space in the sprite vectors and in the vertex vectors
		for (auto &b : _sprites)
			b.second.clear();

		for (auto &v : _vertex)
			v.second.clear();

		_used_ids.clear();
	}

	typename SpriteBatch::sprite_id SpriteBatch::createSprite()
	{
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);

		auto id = _id_count;
		//store the it for later lookup
		_used_ids.push_back(id);

		//TODO: insert into temp/empty sprite storage?
		//? reserve the first batch for unset sprites?

		return id;
	}

	bool SpriteBatch::exists(typename SpriteBatch::sprite_id id) const
	{
		auto lk = std::shared_lock<std::shared_mutex>(_collectionMutex);
		return std::find(std::begin(_used_ids), std::end(_used_ids), id) != std::end(_used_ids);
	}

	void SpriteBatch::setAnimation(typename SpriteBatch::sprite_id id, const resources::animation *a, sf::Time t)
	{
		bool needs_move = false;

		{
			std::shared_lock<std::shared_mutex> lk(_collectionMutex);
			auto found = sprite_utility::FindSprite(_sprites, id);
			auto batch = std::get<0>(found);

			needs_move = _sprites[batch].first.texture != a->tex;
		}

		//prepare both kinds of lock
		std::shared_lock<std::shared_mutex> shlk(_collectionMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> ulk(_collectionMutex, std::defer_lock);

		//lock in the mode that we need
		if (needs_move)
			ulk.lock();
		else
			shlk.lock();

		//update the sprites animation and progress
		auto[batch, sprite_index, sprite] = sprite_utility::FindSprite(_sprites, id);
		{
			std::lock_guard<std::mutex> lk(sprite->mut);
			sprite->animation = a;
			sprite->animation_progress = t;
		}

		//move the sprite to a different batch if the textures no longer match
		if (needs_move)
		{
			auto settings = _sprites[batch].first;
			settings.texture = a->tex;

			sprite_utility::MoveSprite(_sprites, batch, sprite_index, settings);
		}
	}

	void SpriteBatch::setLayer(typename SpriteBatch::sprite_id id, sprite_utility::layer_t l)
	{
		{
			std::shared_lock<std::shared_mutex> lk(_collectionMutex);
			auto found = sprite_utility::FindSprite(_sprites, id);
			auto batch = std::get<0>(found);

			//if we're already in a batch with the correct layer, then don't do anything
			if (_sprites[batch].first.layer == l)
				return;
		}

		//take an exclusive lock so we can rotate the sprite data structure safely
		std::unique_lock<std::shared_mutex> ulk(_collectionMutex);

		//update the sprites animation and progress
		auto found = sprite_utility::FindSprite(_sprites, id);
		auto batch_index = std::get<0>(found);
		auto sprite_index = std::get<1>(found);

		//move the sprite to a batch that matches the new settings
		auto settings = _sprites[batch_index].first;
		sprite_utility::MoveSprite(_sprites, batch_index, sprite_index, settings);
	}

	void SpriteBatch::setPosition(typename SpriteBatch::sprite_id id, sf::Vector2f pos)
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		auto found_sprite = sprite_utility::FindSprite(_sprites, id);
		auto sprite = std::get<sprite_utility::Sprite*>(found_sprite);

		std::lock_guard<std::mutex> slk(sprite->mut);
		sprite->position = pos;
	}

	void SpriteBatch::setSize(typename SpriteBatch::sprite_id id, sf::Vector2f size)
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		auto found_sprite = sprite_utility::FindSprite(_sprites, id);
		auto sprite = std::get<sprite_utility::Sprite*>(found_sprite);

		std::lock_guard<std::mutex> slk(sprite->mut);
		sprite->size = size;
	}

	void SpriteBatch::prepare()
	{
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);

		//clear the previous frames vertex
		for (auto &v : _vertex)
			v.second.clear();

		//
	}

	void SpriteBatch::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (auto &s : _vertex)
		{
			auto &settings = s.first;
			auto state = states;
			state.texture = &settings.texture->value;
			if (settings.shader)
				state.shader = &settings.shader->value;

			target.draw(s.second.data(), s.second.size(), sf::PrimitiveType::Triangles, state);
		}
	}
}

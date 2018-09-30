#include "Hades/SpriteBatch.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "Hades/Animation.hpp"
#include "hades/exceptions.hpp"
#include "Hades/texture.hpp"
#include "hades/Types.hpp"
#include "hades/utility.hpp"

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

		Sprite::Sprite(const Sprite &other) : id(other.id), position(other.position),
			size(other.size), animation(other.animation), animation_progress(other.animation_progress)
		{}

		Sprite &Sprite::operator=(const Sprite &other)
		{
			id = other.id;
			position = other.position;
			size = other.size;
			animation = other.animation;
			animation_progress = other.animation_progress;

			return *this;
		}

		bool operator==(const Sprite &lhs, const Sprite &rhs)
		{
			return lhs.id == rhs.id;
		}

		//must own a shared lock on sbatch
		//throws hades::invalid_argument if id doesn't corrispond to a stored sprite
		found_sprite FindSprite(std::vector<batch> &sbatch, Sprite::sprite_id id)
		{
			for (auto batch_index = 0u; batch_index < sbatch.size(); ++batch_index)
			{
				for (auto sprite_index = 0u; sprite_index < sbatch[batch_index].second.size(); ++sprite_index)
				{
					if (auto &sprite = sbatch[batch_index].second[sprite_index]; sprite.id == id)
					{
						return { batch_index, sprite_index, &sprite };
					}
				}
			}

			throw invalid_argument("Sprite not found in SpriteBatch, id was: " + to_string(id));
		}

		//requires exclusive lock on sbatch
		//moves the sprite to a batch that matches settings, or creates a new one;
		//batch and sprite indexs' indicate where the sprite currently is
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
			}

			//if no batch was found, then make one
			if (!batch)
			{
				sbatch.push_back({ settings,{} });
				batch = &sbatch.back();
			}

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

		auto id = ++_id_count;
		//store the id for later lookup
		_used_ids.push_back(id);

		static const sprite_utility::SpriteSettings settings;

		batch *b = nullptr;

		for (auto &batch : _sprites)
		{
			if (batch.first == settings)
			{
				b = &batch;
				break;
			}
		}

		if (!b)
		{
			_sprites.push_back({ settings, std::vector<sprite_utility::Sprite>() });
			b = &_sprites.back();
		}

		sprite_utility::Sprite s;
		s.id = id;

		//insert sprites into the empty batch
		b->second.push_back(s);

		return id;
	}

	typename SpriteBatch::sprite_id SpriteBatch::createSprite(const resources::animation *a, sf::Time t,
		sprite_utility::layer_t l, sf::Vector2f p, sf::Vector2f s)
	{
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);

		auto id = ++_id_count;
		//store the id for later lookup
		_used_ids.push_back(id);

		sprite_utility::SpriteSettings settings{ l };
		if (a)
			settings.texture = a->tex;

		batch *b = nullptr;

		for (auto &batch : _sprites)
		{
			if (batch.first == settings)
			{
				b = &batch;
				break;
			}
		}

		if (!b)
		{
			_sprites.push_back({ settings, std::vector<sprite_utility::Sprite>() });
			b = &_sprites.back();
		}

		sprite_utility::Sprite spr;
		spr.id = id;
		spr.position = p;
		spr.animation = a;
		spr.animation_progress = t;
		spr.size = s;

		b->second.push_back(spr);

		return id;
	}

	bool SpriteBatch::exists(typename SpriteBatch::sprite_id id) const
	{
		std::shared_lock<std::shared_mutex> lk(_collectionMutex);
		return std::find(std::begin(_used_ids), std::end(_used_ids), id) != std::end(_used_ids);
	}

	void SpriteBatch::destroySprite(sprite_id id)
	{
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);
		auto it = std::find(std::begin(_used_ids), std::end(_used_ids), id);
		if (it == std::end(_used_ids))
			throw invalid_argument("id was not found within the SpriteBatch id list, cannot remove");

		_used_ids.erase(it);

		auto found = sprite_utility::FindSprite(_sprites, id);
		auto batch = std::get<0>(found);
		auto sprite = std::get<sprite_utility::Sprite*>(found);

		auto &sprite_vector = _sprites[batch].second;
		auto sprite_iter = std::find(std::begin(sprite_vector), std::end(sprite_vector), *sprite);

		if(sprite_iter == std::end(sprite_vector))
			throw invalid_argument("id was not found within the SpriteBatch sprite stage, cannot remove");

		sprite_vector.erase(sprite_iter);
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

	void SpriteBatch::limitDrawTo(const sf::FloatRect &worldCoords)
	{
		_drawArea = worldCoords;
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

	using PolySquare = std::array<sf::Vertex, 6>;
	
	PolySquare MakeSquare(const sprite_utility::Sprite &s)
	{
		static const auto col = sf::Color::Cyan;
		//make a coloured rect
		return PolySquare{
			//first triange
			sf::Vertex{ s.position, col }, //top left
			sf::Vertex{ {s.position.x + s.size.x, s.position.y}, col }, //top right
			sf::Vertex{ {s.position.x, s.position.y + s.size.y}, col}, //bottom left
			//second triange
			sf::Vertex{ { s.position.x + s.size.x, s.position.y }, col }, //top right
			sf::Vertex{ { s.position.x + s.size.x, s.position.y + s.size.y }, col }, //bottom right
			sf::Vertex{ { s.position.x, s.position.y + s.size.y }, col } //bottom left
		};
	}

	PolySquare MakeSquareAnimation(const sprite_utility::Sprite &s)
	{
		auto a = s.animation;
		auto [x, y] = animation::GetFrame(a, s.animation_progress);
		return PolySquare{
			//first triange
			sf::Vertex{ s.position, { x, y } }, //top left
			sf::Vertex{ { s.position.x + s.size.x, s.position.y }, { x + a->width, y } }, //top right
			sf::Vertex{ { s.position.x, s.position.y + s.size.y }, { x, y + a->height } }, //bottom left
			//second triange
			sf::Vertex{ { s.position.x + s.size.x, s.position.y },{ x + a->width, y } }, //top right
			sf::Vertex{ { s.position.x + s.size.x, s.position.y + s.size.y },  { x + a->width, y + a->height } }, //bottom right
			sf::Vertex{ { s.position.x, s.position.y + s.size.y },  {x, y + a->height } } //bottom left
		};
	}

	void SpriteBatch::prepare()
	{
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);

		//clear the previous frames vertex
		for (auto &v : _vertex)
			v.second.clear();

		//sort the sprite lists by layer
		std::sort(std::begin(_sprites), std::end(_sprites), [](const batch &lhs, const batch &rhs)
		{return lhs.first.layer < rhs.first.layer; });

		//for each sprite list, write over a vertex buffer 
		//with the settings and vertex data
		for (auto i = 0u; i != _sprites.size(); ++i)
		{
			std::vector<sf::Vertex> *buffer = nullptr;

			//use a vector that already exists in the _vertex vector
			if (i < _vertex.size())
			{
				_vertex[i].first = _sprites[i].first;
				buffer = &_vertex[i].second;
			}
			else
			{
				//if we've exceeded the vectors in _vertex then add a new one
				_vertex.push_back({ _sprites[i].first, {} });
				buffer = &_vertex.back().second;
			}

			assert(buffer);

			//make the vertex data
			for (const auto &s : _sprites[i].second)
			{
				//only test for draw culling if the draw area has been set
				if (_drawArea != sf::FloatRect())
				{
					auto sprite_bounds = sf::FloatRect{ s.position, s.size };

					//skip if the sprite isn't in the draw area
					if (!sprite_bounds.intersects(_drawArea))
						continue;
				}

				PolySquare vertex;
				if (s.animation && s.animation->tex)
					vertex = MakeSquareAnimation(s);
				else //if the sprite has no animation then generated a untextured square
					vertex = MakeSquare(s);

				std::copy(std::begin(vertex), std::end(vertex), std::back_inserter(*buffer));
			}
		}
	}

	void SpriteBatch::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		std::lock_guard<std::shared_mutex> lk(_collectionMutex);
		for (auto &va : _vertex)
		{
			auto &settings = va.first;
			auto state = states;
			if (settings.texture)
				state.texture = &settings.texture->value;
			if (settings.shader)
				state.shader = &settings.shader->value;

			target.draw(va.second.data(), va.second.size(), sf::PrimitiveType::Triangles, state);
		}
	}
}

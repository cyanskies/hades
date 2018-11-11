#include "hades/sprite_batch.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/animation.hpp"
#include "hades/exceptions.hpp"
#include "hades/texture.hpp"
#include "hades/types.hpp"
#include "hades/utility.hpp"

namespace hades
{
	namespace sprite_utility
	{
		bool operator==(const sprite_settings &lhs, const sprite_settings &rhs)
		{
			return lhs.layer == rhs.layer
				&& lhs.texture == rhs.texture
				&& lhs.shader == rhs.shader;
		}

		sprite::sprite(const sprite &other) : id{ other.id }, position{ other.position },
			size{ other.size }, animation{ other.animation }, animation_progress{ other.animation_progress }
		{}

		sprite &sprite::operator=(const sprite &other)
		{
			id = other.id;
			position = other.position;
			size = other.size;
			animation = other.animation;
			animation_progress = other.animation_progress;

			return *this;
		}

		bool operator==(const sprite &lhs, const sprite &rhs)
		{
			return lhs.id == rhs.id;
		}

		//must own a shared lock on sbatch
		//throws hades::invalid_argument if id doesn't corrispond to a stored sprite
		static found_sprite find_sprite(std::vector<batch> &sbatch, sprite::sprite_id id)
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

			throw invalid_argument("Sprite not found in sprite_batch, id was: " + to_string(id));
		}

		//requires exclusive lock on sbatch
		//moves the sprite to a batch that matches settings, or creates a new one;
		//batch and sprite indexs' indicate where the sprite currently is
		static void move_sprite(std::vector<batch> sbatch, index_type batch_index, index_type sprite_index, const sprite_settings &settings)
		{
			//get the sprite out of its current batch
			auto *batch = &sbatch[batch_index];
			const auto sprite = batch->second[sprite_index];

			//erase the sprite from the old batch
			const auto iter = std::begin(batch->second) + sprite_index;
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

	using namespace sprite_utility;

	void sprite_batch::clear()
	{
		//get the exclusive lock
		const auto lk = std::scoped_lock{ _collection_mutex };

		//clear all the vectors
		//NOTE: we don't clear the base vectors, we want to keep the reserved 
		//space in the sprite vectors and in the vertex vectors
		for (auto &b : _sprites)
			b.second.clear();

		for (auto &v : _vertex)
			v.second.clear();

		_used_ids.clear();
	}

	typename sprite_batch::sprite_id sprite_batch::create_sprite()
	{
		const auto lk = std::scoped_lock{ _collection_mutex };

		const auto id = ++_id_count;
		//store the id for later lookup
		_used_ids.push_back(id);

		batch *b = nullptr;

		for (auto &batch : _sprites)
		{
			if (batch.first == sprite_settings{})
			{
				b = &batch;
				break;
			}
		}

		if (!b)
		{
			_sprites.push_back({ sprite_settings{}, std::vector<sprite_utility::sprite>{} });
			b = &_sprites.back();
		}

		sprite_utility::sprite s;
		s.id = id;

		//insert sprites into the empty batch
		b->second.push_back(s);

		return id;
	}

	typename sprite_batch::sprite_id sprite_batch::create_sprite(const resources::animation *a, time_point t,
		sprite_utility::layer_t l, vector_float p, vector_float s)
	{
		const auto lk = std::scoped_lock{ _collection_mutex };

		const auto id = ++_id_count;
		//store the id for later lookup
		_used_ids.push_back(id);

		sprite_utility::sprite_settings settings{ l };
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
			_sprites.push_back({ settings, std::vector<sprite_utility::sprite>{} });
			b = &_sprites.back();
		}

		sprite_utility::sprite spr;
		spr.id = id;
		spr.position = p;
		spr.animation = a;
		spr.animation_progress = t;
		spr.size = s;

		b->second.push_back(spr);

		return id;
	}

	bool sprite_batch::exists(typename sprite_batch::sprite_id id) const
	{
		const auto lock = std::shared_lock{ _collection_mutex };
		return std::find(std::begin(_used_ids), std::end(_used_ids), id) != std::end(_used_ids);
	}

	void sprite_batch::destroy_sprite(sprite_id id)
	{
		const auto lock = std::scoped_lock{ _collection_mutex };
		const auto it = std::find(std::begin(_used_ids), std::end(_used_ids), id);
		if (it == std::end(_used_ids))
			throw invalid_argument("id was not found within the sprite_batch id list, cannot remove");

		_used_ids.erase(it);

		const auto found = sprite_utility::find_sprite(_sprites, id);
		const auto batch = std::get<0>(found);
		const auto sprite = std::get<sprite_utility::sprite*>(found);

		auto &sprite_vector = _sprites[batch].second;
		const auto sprite_iter = std::find(std::begin(sprite_vector), std::end(sprite_vector), *sprite);

		if(sprite_iter == std::end(sprite_vector))
			throw invalid_argument("id was not found within the sprite_batch sprite stage, cannot remove");

		sprite_vector.erase(sprite_iter);
	}

	template<bool Move, template<typename> typename LockType, typename Mutex>
	inline void set_animation_imp(Mutex &mut, std::vector<batch> &sprites,
		typename sprite_batch::sprite_id id, const resources::animation *a, time_point t)
	{
		const auto lock = LockType<Mutex>{ mut };

		//update the sprites animation and progress
		auto[batch, sprite_index, sprite] = sprite_utility::find_sprite(sprites, id);
		{
			const auto lock = std::scoped_lock{ sprite->mut };
			sprite->animation = a;
			sprite->animation_progress = t;
		}

		//move the sprite to a different batch if the textures no longer match
		if constexpr (Move)
		{
			auto settings = sprites[batch].first;
			settings.texture = a->tex;

			sprite_utility::move_sprite(sprites, batch, sprite_index, settings);
		}
	}

	void sprite_batch::set_animation(typename sprite_batch::sprite_id id, const resources::animation *a, time_point t)
	{
		bool needs_move = false;

		{
			const auto lock = std::shared_lock{ _collection_mutex };
			const auto found = sprite_utility::find_sprite(_sprites, id);
			const auto batch = std::get<0>(found);

			needs_move = _sprites[batch].first.texture != a->tex;
		}

		//lock in the mode that we need
		if (needs_move)
			set_animation_imp<true, std::scoped_lock>(_collection_mutex, _sprites, id, a, t);
		else
			set_animation_imp<false, std::shared_lock>(_collection_mutex, _sprites, id, a, t);	
	}

	void sprite_batch::set_layer(typename sprite_batch::sprite_id id, sprite_utility::layer_t l)
	{
		{
			const auto lock = std::shared_lock{ _collection_mutex };
			const auto found = sprite_utility::find_sprite(_sprites, id);
			const auto batch = std::get<0>(found);

			//if we're already in a batch with the correct layer, then don't do anything
			if (_sprites[batch].first.layer == l)
				return;
		}

		//take an exclusive lock so we can rotate the sprite data structure safely
		const auto lock = std::scoped_lock{ _collection_mutex };

		//update the sprites animation and progress
		const auto found = sprite_utility::find_sprite(_sprites, id);
		const auto batch_index = std::get<0>(found);
		const auto sprite_index = std::get<1>(found);

		//move the sprite to a batch that matches the new settings
		const auto settings = _sprites[batch_index].first;
		sprite_utility::move_sprite(_sprites, batch_index, sprite_index, settings);
	}

	void sprite_batch::set_position(typename sprite_batch::sprite_id id, vector_float pos)
	{
		const auto lock = std::shared_lock{ _collection_mutex };
		auto found_sprite = sprite_utility::find_sprite(_sprites, id);
		auto sprite = std::get<sprite_utility::sprite*>(found_sprite);

		const auto sprite_lock = std::scoped_lock{ sprite->mut };
		sprite->position = pos;
	}

	void sprite_batch::set_size(typename sprite_batch::sprite_id id, vector_float size)
	{
		const auto lock = std::shared_lock{ _collection_mutex };
		auto found_sprite = sprite_utility::find_sprite(_sprites, id);
		auto sprite = std::get<sprite_utility::sprite*>(found_sprite);

		const auto sprite_lock = std::scoped_lock{ sprite->mut };
		sprite->size = size;
	}

	void sprite_batch::draw_clamp(const rect_float &r)
	{
		_draw_clamp = r;
	}

	using poly_square = std::array<sf::Vertex, 6>;
	
	static poly_square make_square(const sprite_utility::sprite &s)
	{
		static const auto col = sf::Color::Cyan;
		//make a coloured rect
		return poly_square{
			//first triange
			sf::Vertex{ {s.position.x, s.position.y}, col }, //top left
			sf::Vertex{ {s.position.x + s.size.x, s.position.y}, col }, //top right
			sf::Vertex{ {s.position.x, s.position.y + s.size.y}, col}, //bottom left
			//second triange
			sf::Vertex{ { s.position.x + s.size.x, s.position.y }, col }, //top right
			sf::Vertex{ { s.position.x + s.size.x, s.position.y + s.size.y }, col }, //bottom right
			sf::Vertex{ { s.position.x, s.position.y + s.size.y }, col } //bottom left
		};
	}

	static poly_square make_square_animation(const sprite_utility::sprite &s)
	{
		const auto a = s.animation;
		const auto [x, y] = animation::get_frame(*a, s.animation_progress);
		return poly_square{
			//first triange
			sf::Vertex{ {s.position.x, s.position.y}, { x, y } }, //top left
			sf::Vertex{ { s.position.x + s.size.x, s.position.y }, { x + a->width, y } }, //top right
			sf::Vertex{ { s.position.x, s.position.y + s.size.y }, { x, y + a->height } }, //bottom left
			//second triange
			sf::Vertex{ { s.position.x + s.size.x, s.position.y },{ x + a->width, y } }, //top right
			sf::Vertex{ { s.position.x + s.size.x, s.position.y + s.size.y },  { x + a->width, y + a->height } }, //bottom right
			sf::Vertex{ { s.position.x, s.position.y + s.size.y },  {x, y + a->height } } //bottom left
		};
	}

	void sprite_batch::prepare()
	{
		const auto lock = std::scoped_lock{ _collection_mutex };

		//clear the previous frames vertex
		for (auto &v : _vertex)
			v.second.clear();

		//sort the sprite lists by layer
		std::sort(std::begin(_sprites), std::end(_sprites), [](const batch &lhs, const batch &rhs)
		{ return lhs.first.layer < rhs.first.layer; });

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
				if (_draw_clamp != rect_float{})
				{
					const auto sprite_bounds = rect_float{ s.position.x, s.position.y, s.size.x, s.size.y };

					//skip if the sprite isn't in the draw area
					if (!intersects(_draw_clamp, sprite_bounds))
						continue;
				}

				poly_square vertex = s.animation && s.animation->tex ? make_square_animation(s) : make_square(s);
				std::move(std::begin(vertex), std::end(vertex), std::back_inserter(*buffer));
			}
		}
	}

	void sprite_batch::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		const auto lock = std::scoped_lock{ _collection_mutex };
		for (auto &va : _vertex)
		{
			const auto &settings = va.first;
			auto state = states;
			if (settings.texture)
				state.texture = &settings.texture->value;
			//if (settings.shader)
			//	state.shader = &settings.shader->value;

			target.draw(va.second.data(), va.second.size(), sf::PrimitiveType::Triangles, state);
		}
	}
}

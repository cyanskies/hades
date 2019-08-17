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
				&& lhs.texture == rhs.texture;
				//&& lhs.shader == rhs.shader;
		}

		sprite::sprite(const sprite &other) noexcept : id{ other.id }, position{ other.position },
			size{ other.size }, animation{ other.animation }, animation_progress{ other.animation_progress }
		{}

		sprite &sprite::operator=(const sprite &other) noexcept
		{
			id = other.id;
			position = other.position;
			size = other.size;
			animation = other.animation;
			animation_progress = other.animation_progress;

			return *this;
		}

		bool operator==(const sprite &lhs, const sprite &rhs) noexcept
		{
			return lhs.id == rhs.id;
		}
	}

	using namespace sprite_utility;

	sprite_batch::sprite_batch(const sprite_batch& other) : _sprites{other._sprites}, _vertex{other._vertex},
		_ids{other._ids}, _id_count{other._id_count}
	{}

	sprite_batch::sprite_batch(sprite_batch&& other) noexcept : _sprites{ std::move(other._sprites) }, _vertex{ std::move(other._vertex) },
		_ids{ std::move(other._ids) }, _id_count{ other._id_count }
	{}

	sprite_batch& sprite_batch::operator=(const sprite_batch&o)
	{
		_sprites = o._sprites;
		_vertex = o._vertex;
		_ids = o._ids;
		_id_count = o._id_count;

		return *this;
	}

	sprite_batch& sprite_batch::operator=(sprite_batch&& o) noexcept
	{
		_sprites = std::move(o._sprites);
		_vertex = std::move(o._vertex);
		_ids = std::move(o._ids);
		_id_count = std::move(o._id_count);

		return *this;
	}

	void sprite_batch::clear()
	{
		_sprites.clear();

		for (auto& v : _vertex)
			v.buffer = quad_buffer{};

		_ids.clear();

		return;
	}

	void sprite_batch::set_async(bool a)
	{
		return;
	}

	void sprite_batch::swap(sprite_batch& rhs) noexcept
	{
		std::swap(_sprites, rhs._sprites);
		std::swap(_vertex, rhs._vertex);
		std::swap(_ids, rhs._ids);
		std::swap(_id_count, rhs._id_count);
		return;
	}

	//NOTE: requires a shared lock on the sprite collection
	static std::size_t find_batch(const sprite_utility::sprite_settings s, const std::deque<sprite_utility::batch> &v) noexcept
	{
		auto index = std::size(v);
		for (auto i = std::size_t{}; i != std::size(v); ++i)
		{
			std::shared_lock{ v[i].mutex };
			if (v[i].settings == s)
			{
				index = i;
				break;
			}
		}

		return index;
	}

	sprite_id sprite_batch::create_sprite()
	{
		sprite_id id;

		{
			const auto lock = std::scoped_lock{ _id_mutex };
			id = increment(_id_count);
			assert(id != bad_sprite_id);
		}

		sprite s;
		s.id = id;

		_add_sprite(s);

		return id;
	}

	typename sprite_batch::sprite_id sprite_batch::create_sprite(const resources::animation *a, time_point t,
		sprite_utility::layer_t l, vector_float p, vector_float s)
	{
		sprite_id id;

		{
			const auto lock = std::scoped_lock{ _id_mutex };
			id = increment(_id_count);
			assert(id != bad_sprite_id);
		}

		sprite sp;
		sp.id = id;
		sp.position = p;
		sp.size = s;
		sp.layer = l;
		sp.animation = a;
		sp.animation_progress = t;

		_add_sprite(sp);

		return id;
	}

	bool sprite_batch::exists(typename sprite_batch::sprite_id id) const noexcept
	{
		const auto lock = std::shared_lock{ _id_mutex };
		for (const auto s : _ids)
		{
			if (s.id == id)
				return true;
		}

		return false;
	}

	void sprite_batch::destroy_sprite(sprite_id id)
	{
		index_t batch_index = _find_sprite(id);

		{
			auto lock1 = std::shared_lock{ _sprites_mutex, std::defer_lock };
			auto lock2 = std::shared_lock{ _vertex_mutex, std::defer_lock };
			const auto lock = std::scoped_lock{ lock1, lock2 };

			auto& s_batch = _sprites[batch_index];
			auto& v_batch = _vertex[batch_index];

			const auto batch_locks = std::scoped_lock{ s_batch.mutex, v_batch.mutex };

			index_t index;
			for (auto i = index_t{}; i < std::size(v_batch.sprites); ++i)
			{
				if (v_batch.sprites[i] == id)
				{
					index = i;
					break;
				}
			}

			const auto last = std::size(v_batch.sprites) - 1;
			if (index != last)
			{
				std::swap(s_batch.sprites[index], s_batch.sprites[last]);
				std::swap(v_batch.sprites[index], v_batch.sprites[last]);
				v_batch.buffer.swap(index, last);
			}

			s_batch.sprites.pop_back();
			v_batch.sprites.pop_back();
			v_batch.buffer.pop_back();
		}

		const auto lock = std::scoped_lock{ _id_mutex };
		for (auto i = std::size_t{}; i < std::size(_ids); ++i)
		{
			if (_ids[i].id == id)
			{
				const auto last = std::size(_ids) - 1;
				std::swap(_ids[i], _ids[last]);
				_ids.pop_back();
				break;
			}
		}

		return;
	}

	void sprite_batch::set_animation(sprite_id id, const resources::animation *a, time_point t)
	{
		_apply_changes(id, [a, t](sprite s) noexcept {
			s.animation = a;
			s.animation_progress = t;
			return s;
		});

		return;
	}

	void sprite_batch::set_position_animation(sprite_id id, vector_float pos, const resources::animation* a, time_point t)
	{
		_apply_changes(id, [pos, a, t](sprite s) noexcept {
			s.position = pos;
			s.animation = a;
			s.animation_progress = t;
			return s;
		});

		return;
	}

	void sprite_batch::set_layer(typename sprite_batch::sprite_id id, sprite_utility::layer_t l)
	{
		_apply_changes(id, [l](sprite s) noexcept {
			s.layer = l;
			return s;
		});

		return;
	}

	void sprite_batch::set_sprite(sprite_id id, const resources::animation* a, time_point t, sprite_utility::layer_t l, vector_float p, vector_float s)
	{
		_apply_changes(id, [a, t, l, p, siz = s](sprite s) noexcept {
			s.animation = a;
			s.animation_progress = t;
			s.layer = l;
			s.position = p;
			s.size = siz;
			return s;
		});

		return;
	}

	void sprite_batch::set_position(typename sprite_batch::sprite_id id, vector_float pos)
	{
		_apply_changes(id, [pos](sprite s) noexcept {
			s.position = pos;
			return s;
		});

		return;
	}

	void sprite_batch::set_size(typename sprite_batch::sprite_id id, vector_float size)
	{
		_apply_changes(id, [size](sprite s) noexcept {
			s.size = size;
			return s;
		});

		return;
	}

	std::vector<sprite_utility::layer_t> sprite_batch::get_layer_list() const
	{
		auto out = std::vector<sprite_utility::layer_t>{};

		for (const auto& s : _sprites)
			out.emplace_back(s.settings.layer);

		return out;
	}

	void sprite_batch::apply()
	{
		for (auto& v : _vertex)
			v.buffer.apply();
		return;
	}

	void sprite_batch::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		const auto layers = get_layer_list();

		struct info {
			layer_t l;
			index_t i;
		};

		auto layer_data = std::vector<info>{};
		layer_data.reserve(std::size(layers));

		for (auto i = std::size_t{}; i < std::size(layers); ++i)
			layer_data.emplace_back(info{ layers[i], i });

		std::sort(std::begin(layer_data), std::end(layer_data), [](info a, info b) {
			return a.l < b.l;
		});

		for (const auto layer : layer_data)
			draw(target, layer.i, states);

		return;
	}

	void sprite_batch::draw(sf::RenderTarget &t, sprite_utility::layer_t l, sf::RenderStates s) const
	{
		for (auto i = std::size_t{}; i < std::size(_sprites); ++i)
		{
			if (_sprites[i].settings.layer == l)
			{
				s.texture = &_sprites[i].settings.texture->value;
				t.draw(_vertex[i].buffer, s);
				return;
			}
		}

		throw sprite_batch_error{ "tried to draw a layer that could not be found" };
	}

	void sprite_batch::draw(sf::RenderTarget& target, index_t layer_index, sf::RenderStates s) const
	{
		assert(layer_index < std::size(_sprites));

		s.texture = &_sprites[layer_index].settings.texture->value;
		target.draw(_vertex[layer_index].buffer, s);
		return;
	}

	sprite_batch::index_t sprite_batch::_find_sprite(sprite_id id) const
	{
		const auto lock = std::shared_lock{ _id_mutex };
		for (const auto s : _ids)
		{
			if (s.id == id)
				return s.index;
		}

		throw sprite_batch_invalid_id{ "tried to find a sprite that isnt in the batch" };
	}

	//remove a sprite from its current batch,
	//insert it into a more appropriate one
	sprite sprite_batch::_remove_sprite(sprite_id id, index_t current, index_t index)
	{
		sprite s;

		{
			auto lock1 = std::shared_lock{ _sprites_mutex, std::defer_lock };
			auto lock2 = std::shared_lock{ _vertex_mutex, std::defer_lock };
			const auto lock = std::scoped_lock{ lock1, lock2 };

			auto* s_batch = &_sprites[current];
			auto* v_batch = &_vertex[current];

			const auto batch_locks = std::scoped_lock{ s_batch->mutex, v_batch->mutex };
			
			//if our index data is stale
			if (index >= std::size(v_batch->sprites) || v_batch->sprites[index] != id)
			{
				//find it again
				index = std::size(v_batch->sprites);
				for (auto i = index_t{}; i < std::size(v_batch->sprites); ++i)
				{
					if (v_batch->sprites[i] == id)
					{
						index = i;
						break;
					}
				}

				//TODO: if index == std::size(v_batch->sprites)
				//	we're not even in the right batch :/
				//	just give up?
			}

			const auto last = std::size(v_batch->sprites) - 1;

			s = s_batch->sprites[index];

			std::swap(s_batch->sprites[index], s_batch->sprites[last]);
			std::swap(v_batch->sprites[index], v_batch->sprites[last]);
			v_batch->buffer.swap(index, last);

			s_batch->sprites.pop_back();
			v_batch->sprites.pop_back();
			v_batch->buffer.pop_back();
		}

		{
			const auto lock = std::scoped_lock{ _id_mutex };
			auto index = std::size(_ids);

			for (auto i = std::size_t{}; i < std::size(_ids); ++i)
			{
				if (_ids[i].id == id)
				{
					index = i;
					break;
				}
			}
			assert(index < std::size(_ids));

			const auto last = std::size(_ids) - 1;

			std::swap(_ids[index], _ids[last]);
			_ids.pop_back();
		}

		return s;
	}

	void sprite_batch::_add_sprite(sprite s)
	{
		index_t index;
		const auto settings = sprite_settings{ s.layer, s.animation->tex };

		{
			const auto share_lock = std::shared_lock{ _sprites_mutex };
			index = find_batch(settings, _sprites);
		}

		if (index == std::size(_sprites))
		{
			const auto lock = std::scoped_lock{ _sprites_mutex, _vertex_mutex };

			//we have to search again, in case someone else just made this batch
			index = find_batch(settings, _sprites);
			if (index == std::size(_sprites))
			{
				_sprites.emplace_back(sprite_utility::batch{ settings, std::vector<sprite_utility::sprite>{} });
				_vertex.emplace_back();
			}
		}

		{
			const auto lock = std::scoped_lock{ _id_mutex };
			_ids.emplace_back(sprite_pos{ s.id, index });
		}

		auto sprite_lock = std::shared_lock{ _sprites_mutex, std::defer_lock };
		auto vertex_lock = std::shared_lock{ _vertex_mutex, std::defer_lock };
		const auto lock = std::scoped_lock(sprite_lock, vertex_lock);

		const auto s_lock = std::scoped_lock{ _sprites[index].mutex,
												_vertex[index].mutex };
		_sprites[index].sprites.emplace_back(s);
		const auto frame = animation::get_frame(*s.animation, s.animation_progress);
		_vertex[index].buffer.append(make_quad_animation(s.position, s.size, *s.animation, frame));
		_vertex[index].sprites.emplace_back(s.id);

		return;
	}
}

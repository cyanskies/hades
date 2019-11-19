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

		bool operator==(const sprite &lhs, const sprite &rhs) noexcept
		{
			return lhs.id == rhs.id;
		}
	}

	using namespace sprite_utility;

	void sprite_batch::clear()
	{
		_sprites.clear();

		for (auto& v : _vertex)
		{
			v.buffer = quad_buffer{};
			v.sprites.clear();
		}

		_ids.clear();

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

	static std::size_t find_batch(const sprite_utility::sprite_settings s, const std::deque<sprite_utility::batch> &v) noexcept
	{
		auto index = std::size(v);
		for (auto i = std::size_t{}; i != std::size(v); ++i)
		{
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
		auto id = increment(_id_count) ;
		assert(id != bad_sprite_id);

		_add_sprite({ id });

		return id;
	}

	typename sprite_batch::sprite_id sprite_batch::create_sprite(const resources::animation *a, time_point t,
		sprite_utility::layer_t l, vector_float p, vector_float s)
	{
		const auto id = increment(_id_count);
		assert(id != bad_sprite_id);

		_add_sprite({ id, p, s, l, a, t });

		return id;
	}

	bool sprite_batch::exists(typename sprite_batch::sprite_id id) const noexcept
	{
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

		auto& s_batch = _sprites[batch_index];
		auto& v_batch = _vertex[batch_index];

		const auto index = [id, &v_batch]()->index_t {
			for (auto i = index_t{}; i < std::size(v_batch.sprites); ++i)
			{
				if (v_batch.sprites[i] == id)
					return i;
			}
			return std::size(v_batch.sprites);
		}();
		assert(index != std::size( v_batch.sprites));

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
	
		//remove from id list
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

	void sprite_batch::set_sprite(sprite_id id, time_point t, vector_float p, vector_float s)
	{
		_apply_changes(id, [t, p , siz = s](sprite s) noexcept {
			s.animation_progress = t;
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

	std::vector<sprite_batch::layer_info> sprite_batch::get_layer_info_list() const
	{
		const auto layers = get_layer_list();
		auto layer_data = std::vector<layer_info>{};
		layer_data.reserve(std::size(layers));

		for (auto i = std::size_t{}; i < std::size(layers); ++i)
			layer_data.emplace_back(layer_info{ layers[i], i });

		std::sort(std::begin(layer_data), std::end(layer_data), [](layer_info a, layer_info b) {
			return a.l < b.l;
			});

		return layer_data;
	}

	void sprite_batch::apply()
	{
		for (auto& v : _vertex)
			v.buffer.apply();
		return;
	}

	void sprite_batch::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		const auto layer_data = get_layer_info_list();
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
				draw(t, i, s);
				return;
			}
		}

		throw sprite_batch_error{ "tried to draw a layer that could not be found" };
	}

	void sprite_batch::draw(sf::RenderTarget& target, index_t layer_index, sf::RenderStates s) const
	{
		assert(layer_index < std::size(_sprites));

		if(_sprites[layer_index].settings.texture)
			s.texture = &_sprites[layer_index].settings.texture->value;
		target.draw(_vertex[layer_index].buffer, s);
		return;
	}

	sprite_batch::index_t sprite_batch::_find_sprite(sprite_id id) const
	{
		for (const auto s : _ids)
		{
			if (s.id == id)
				return s.index;
		}

		throw sprite_batch_invalid_id{ "tried to find a sprite that isnt in the batch" };
	}

	//remove a sprite from its current batch,
	sprite sprite_batch::_remove_sprite(sprite_id id, index_t current, index_t index)
	{
		auto& s_batch = _sprites[current];
		auto& v_batch = _vertex[current];

		const auto last = std::size(v_batch.sprites) - 1;

		const auto s = s_batch.sprites[index];

		//remove sprite
		if (index != last)
		{
			std::swap(s_batch.sprites[index], s_batch.sprites[last]);
			std::swap(v_batch.sprites[index], v_batch.sprites[last]);
			v_batch.buffer.swap(index, last);
		}

		s_batch.sprites.pop_back();
		v_batch.sprites.pop_back();
		v_batch.buffer.pop_back();
	
		//reinsert
		const auto id_index = [id, &_ids = _ids]() {
			for (auto i = std::size_t{}; i < std::size(_ids); ++i)
			{
				if (_ids[i].id == id)
					return i;
			}
			return std::size(_ids);
		}();
		assert(id_index != std::size(_ids));

		const auto id_last = std::size(_ids) - 1;
		std::swap(_ids[id_index], _ids[id_last]);
		_ids.pop_back();
	
		return s;
	}

	void sprite_batch::_add_sprite(sprite s)
	{
		const auto settings = sprite_settings{ s.layer,
			s.animation ? s.animation->tex : nullptr };
		const index_t index = find_batch(settings, _sprites);

		//no batch matches the desired settings
		if (index == std::size(_sprites))
		{
			_sprites.emplace_back(sprite_utility::batch{ settings, std::vector<sprite_utility::sprite>{} });
			_vertex.emplace_back();
		}

		_ids.emplace_back(sprite_pos{ s.id, index });

		_sprites[index].sprites.emplace_back(s);
		if (s.animation)
		{
			const auto frame = animation::get_frame(*s.animation, s.animation_progress);
			_vertex[index].buffer.append(make_quad_animation(s.position, s.size, *s.animation, frame));
		}
		else
			_vertex[index].buffer.append(make_quad_colour({ s.position, s.size }, colours::white));

		_vertex[index].sprites.emplace_back(s.id);

		return;
	}
}

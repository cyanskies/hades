#include "hades/sprite_batch.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"
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
				&& lhs.shader_proxy == rhs.shader_proxy;
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
			v.buffer.clear();
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

	static sprite_batch::index_t find_batch(const sprite_utility::sprite_settings& s, const std::vector<sprite_utility::batch> &v) noexcept
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
		const auto id = increment(_id_count) ;
		assert(id != bad_sprite_id);

        _add_sprite({ id, {}, {}, {}, {}, {} });

		return id;
	}

	typename sprite_batch::sprite_id sprite_batch::create_sprite(const resources::animation *a, time_point t,
		sprite_utility::layer_t l, vector2_float p, vector2_float s, const resources::shader_uniform_map *u)
	{
		const auto id = increment(_id_count);
		assert(id != bad_sprite_id);

		auto spri = sprite{ id, p, s, a, t };

		spri.settings.layer = l;

		if (a)
		{
			spri.settings.texture = resources::animation_functions::get_texture(*a);
			if (resources::animation_functions::has_shader(*a))
			{
				spri.settings.shader_proxy = resources::animation_functions::get_shader_proxy(*a);
				if (u)
					spri.settings.shader_proxy->set_uniforms(*u);
			}
			else
				spri.settings.shader_proxy = {};
		}
		else
		{
			spri.settings.texture = {};
			spri.settings.shader_proxy = {};
		}

		_add_sprite(std::move(spri));

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

	static sprite_batch::index_t find_id_index(sprite_id id, const std::vector<sprite_batch::sprite_pos>& ids)
	{
		const auto end = std::size(ids);
		for (auto i = std::size_t{}; i < end; ++i)
		{
			if (ids[i].id == id)
				return i;
		}

		return size(ids);
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
		auto id_index = find_id_index(id, _ids);
		assert(id_index != std::numeric_limits<index_t>::max());

		const auto back = std::size(_ids) - 1;
		std::swap(_ids[id_index], _ids[back]);
		_ids.pop_back();
		return;
	}

	void sprite_batch::set_animation(sprite_id id, const resources::animation *a, time_point t)
	{
		auto& s = _get_sprite(id);
		s.animation = a;

		if (a)
		{
			s.settings.texture = resources::animation_functions::get_texture(*a);
			if (resources::animation_functions::has_shader(*a))
				s.settings.shader_proxy = resources::animation_functions::get_shader_proxy(*a);
			else
				s.settings.shader_proxy = {};
		}
		else
		{
			s.settings.texture = {};
			s.settings.shader_proxy = {};
		}

		s.animation_progress = t;
		return;
	}

	void sprite_batch::set_animation(sprite_id id, time_point t)
	{
		auto& s = _get_sprite(id);
		s.animation_progress = t;
		return;
	}

	void sprite_batch::set_position_animation(sprite_id id, vector2_float pos, const resources::animation* a, time_point t)
	{
		auto& s = _get_sprite(id);
		s.position = pos;
		s.animation = a;

		if (a)
		{
			s.settings.texture = resources::animation_functions::get_texture(*a);
			if (resources::animation_functions::has_shader(*a))
				s.settings.shader_proxy = resources::animation_functions::get_shader_proxy(*a);
			else
				s.settings.shader_proxy = {};
		}
		else
		{
			s.settings.texture = {};
			s.settings.shader_proxy = {};
		}

		s.animation_progress = t;
		return;
	}

	void sprite_batch::set_layer(typename sprite_batch::sprite_id id, sprite_utility::layer_t l)
	{
		auto& s = _get_sprite(id);
		s.settings.layer = l;
		return;
	}

	void sprite_batch::set_sprite(sprite_id id, const resources::animation* a,
		time_point t, sprite_utility::layer_t l, vector2_float p, vector2_float siz,
		const resources::shader_uniform_map *u)
	{
		auto& s = _get_sprite(id);
		s.animation = a;

		if (a)
		{
			s.settings.texture = resources::animation_functions::get_texture(*a);
			if (resources::animation_functions::has_shader(*a))
			{
				s.settings.shader_proxy = resources::animation_functions::get_shader_proxy(*a);
				if (u)
					s.settings.shader_proxy->set_uniforms(*u);
			}
			else
				s.settings.shader_proxy = {};
		}
		else
		{
			s.settings.texture = {};
			s.settings.shader_proxy = {};
		}

		s.animation_progress = t;
		s.settings.layer = l;
		s.position = p;
		s.size = siz;
		return;
	}

	void sprite_batch::set_sprite(sprite_id id, time_point t, vector2_float p, vector2_float siz)
	{
		auto& s = _get_sprite(id);
		s.animation_progress = t;
		s.position = p;
		s.size = siz;
		return;
	}

	void sprite_batch::set_position(typename sprite_batch::sprite_id id, vector2_float pos)
	{
		auto& s = _get_sprite(id);
		s.position = pos;
		return;
	}

	void sprite_batch::set_size(typename sprite_batch::sprite_id id, vector2_float size)
	{
		auto& s = _get_sprite(id);
		s.size = size;
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
		_apply_changes();
		_remove_empty_batch();

		for (auto& v : _vertex)
			v.buffer.apply();
		return;
	}

	void sprite_batch::draw(sf::RenderTarget& target, const sf::RenderStates& states) const
	{
		const auto layer_data = get_layer_info_list();
		for (const auto layer : layer_data)
			draw(target, layer.i, states);

		return;
	}

	void sprite_batch::draw(sf::RenderTarget& target, index_t layer_index, const sf::RenderStates& states) const
	{
		assert(layer_index < std::size(_sprites));
		auto s = states;
		auto& settings = _sprites[layer_index].settings;
		if(settings.texture)
			s.texture = &resources::texture_functions::get_sf_texture(_sprites[layer_index].settings.texture);
		if (settings.shader_proxy)
			s.shader = settings.shader_proxy->get_shader();
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

	sprite_utility::sprite& sprite_batch::_get_sprite(sprite_id id)
	{
		const auto index = _find_sprite(id);
		auto& s_batch = _sprites[index];
		auto iter = std::ranges::find(s_batch.sprites, id, &sprite_utility::sprite::id);
		assert(iter != end(s_batch.sprites)); // _find_sprite should have thrown if the sprite is missing
		return *iter;
	}

	// removes sprite data from batches without erasing the id data
	static sprite_utility::sprite remove_sprite(sprite_batch::index_t index, sprite_batch::index_t buffer_index,
		std::vector<sprite_utility::batch>& sprites, std::vector<sprite_batch::vert_batch>& verts)
	{
		auto& s_batch = sprites[index];
		auto& v_batch = verts[index];

		const auto last = std::size(v_batch.sprites) - 1;

		const auto s = std::move(s_batch.sprites[buffer_index]);

		//remove sprite
		std::swap(s_batch.sprites[buffer_index], s_batch.sprites[last]);
		std::swap(v_batch.sprites[buffer_index], v_batch.sprites[last]);
		v_batch.buffer.swap(buffer_index, last);

		s_batch.sprites.pop_back();
		v_batch.sprites.pop_back();
		v_batch.buffer.pop_back();
		return s;
	}

	//remove a sprite from its current batch,
	sprite sprite_batch::_remove_sprite(sprite_id id, index_t current, index_t index)
	{
		const auto s = remove_sprite(current, index, _sprites, _vertex);
		
		//remove from id list
		auto id_index = find_id_index(id, _ids);
		assert(id_index < size(_ids));

		const auto back = std::size(_ids) - 1;
		std::swap(_ids[id_index], _ids[back]);
		_ids.pop_back();
		return s;
	}

	void sprite_batch::_apply_changes()
	{
		namespace animf = resources::animation_functions;
		
		for (auto& sprite : _ids)
		{
			const auto id = sprite.id;
			const auto index = sprite.index;
						
			auto& s_batch = _sprites[index];
			auto& v_batch = _vertex[index];

			// find sprite batch
			const auto s_index = [id, &v_batch]() {
				for (auto i = index_t{}; i < std::size(v_batch.sprites); ++i)
				{
					if (v_batch.sprites[i] == id)
						return i;
				}
				return std::size(v_batch.sprites);
			}();
			assert(s_index != std::size(v_batch.sprites));

			// NOTE: this reference may be invalidated below
			//		don't access it after moving the sprite
			const auto& s = s_batch.sprites[s_index];
			assert(s.id == id);

			if (s.settings == s_batch.settings)
			{
				// NOTE: this code is duplicated in insert_sprite
				if (s.animation)
				{
					const auto& frame = animation::get_frame(*s.animation, s.animation_progress);
					_vertex[index].buffer.replace(make_quad_animation(s.position, s.size, frame), s_index);
				}
				else
					_vertex[index].buffer.replace(make_quad_colour({ s.position, s.size }, colours::from_name(colours::names::white)), s_index);
			}
			else
			{
				//if we reach here, then we need to move to a new batch
				_reseat_sprite(id, sprite, s_index);
			}
		}

		return;
	}

	void sprite_batch::_remove_empty_batch() noexcept
	{
		// find first empty batch
		const auto iter = std::ranges::find_if(_sprites, [](const auto& s) {
			return empty(s.sprites);
			});

		if (iter == end(_sprites))
			return;

		// remove batch
		const auto index = std::distance(begin(_sprites), iter);
		const auto vert_iter = std::next(begin(_vertex), index);
		_sprites.erase(iter);
		_vertex.erase(vert_iter);

		// fix indicies
		for (auto& s : _ids)
		{
			assert(std::cmp_not_equal(s.index, index));
			if (std::cmp_greater(s.index, index))
				--s.index;
		}

		return;
	}

	// inserts a sprite into the sprite and vertex arrays
	static sprite_batch::index_t insert_sprite(sprite s,
		std::vector<sprite_utility::batch>& sprites, std::vector<sprite_batch::vert_batch>& verts)
	{
		namespace anims = resources::animation_functions;
		const auto index = find_batch(s.settings, sprites);

		//no batch matches the desired settings
		if (index == std::size(sprites))
		{
			if (s.animation && !anims::is_loaded(*s.animation))
				anims::get_resource(anims::get_id(*s.animation)); // force lazy load

			sprites.emplace_back(s.settings, std::vector<sprite_utility::sprite>{});
			verts.emplace_back();
		}

		const auto& spr = sprites[index].sprites.emplace_back(std::move(s));
		// NOTE: this code is duplicated in _apply_changes
		if (spr.animation)
		{
			const auto& frame = animation::get_frame(*spr.animation, spr.animation_progress);
			verts[index].buffer.append(make_quad_animation(spr.position, spr.size, frame));
		}
		else
			verts[index].buffer.append(make_quad_colour({ spr.position, spr.size }, colours::from_name(colours::names::white)));

		verts[index].sprites.emplace_back(spr.id);
		return index;
	}

	void sprite_batch::_add_sprite(sprite s)
	{
		const auto index = insert_sprite(std::move(s), _sprites, _vertex);
		_ids.emplace_back(sprite_pos{ s.id, index });
		return;
	}

	void sprite_batch::_reseat_sprite(sprite_id, sprite_pos& pos, index_t buffer_index)
	{
		auto s = remove_sprite(pos.index, buffer_index, _sprites, _vertex);
		pos.index = insert_sprite(std::move(s), _sprites, _vertex);
		return;
	}
}

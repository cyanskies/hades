namespace hades::resources
{
	template<typename Func>
	void call_with_uniform_type(uniform_type_list t, Func&& f)
	{
		switch (t)
		{
		case uniform_type_list::float_t:
			return static_cast<void>(f.template operator()<float>());
		case uniform_type_list::vector2f:
			return static_cast<void>(f.template operator()<vector2_float>());
		case uniform_type_list::vector3f:
			return static_cast<void>(f.template operator()<vector3<float>>());
		case uniform_type_list::vector4f:
			return static_cast<void>(f.template operator()<vector4<float>>()); 
		case uniform_type_list::int_t:
			return static_cast<void>(f.template operator()<int>());
		case uniform_type_list::vector2i:
			return static_cast<void>(f.template operator()<vector2_int>());
		case uniform_type_list::vector3i:
			return static_cast<void>(f.template operator()<vector3<int>>());
		case uniform_type_list::vector4i:
			return static_cast<void>(f.template operator()<vector4<int>>());
		case uniform_type_list::matrix3:
			return static_cast<void>(f.template operator()<sf::Glsl::Mat3>());
		case uniform_type_list::matrix4:
			return static_cast<void>(f.template operator()<sf::Glsl::Mat4>());
		case uniform_type_list::texture:
			return static_cast<void>(f.template operator()<const texture*>());
		}

		throw unexpected_uniform{ "Unrecognised uniform type" };
	}

	template<uniform_type T>
	void shader_proxy::set_uniform(std::string_view name, const T& t)
	{
		assert(_shader);
		auto iter = _uniforms->find(name);
		if (iter == end(*_uniforms))
		{
			throw unexpected_uniform{ 
				std::format("Tried to set an unexpected uniform: {}, on shader: {}",
				name,
				to_string(shader_functions::get_id(*_shader))) 
			};
		}

		//check uniform type
		call_with_uniform_type(iter->second.type, [&]<uniform_type U>() {
			if (!std::same_as<T, U>)
			{
				throw uniform_wrong_type{
					std::format("Called set_uniform with wrong type for uniform: {}, expected: {}, provided: {}",
					name, typeid(U).name(), typeid(T).name())
						};
			}
		});
		
		auto& shad = shader_functions::get_shader(*_shader);

		if constexpr (std::same_as<T, vector2_float>)
			shad.setUniform(iter->first, sf::Glsl::Vec2{ t.x, t.y });
		else if constexpr (std::same_as<T, vector3<float>>)
			shad.setUniform(iter->first, sf::Glsl::Vec3{ t.x, t.y, t.z });
		else if constexpr (std::same_as<T, vector4<float>>)
			shad.setUniform(iter->first, sf::Glsl::Vec4{ t.x, t.y, t.z, t.w });
		else if constexpr (std::same_as<T, vector2_int>)
			shad.setUniform(iter->first, sf::Glsl::Ivec2{ t.x, t.y });
		else if constexpr (std::same_as<T, vector3<int>>)
			shad.setUniform(iter->first, sf::Glsl::Ivec3{ t.x, t.y, t.z });
		else if constexpr (std::same_as<T, vector4<int>>)
			shad.setUniform(iter->first, sf::Glsl::Ivec4{ t.x, t.y, t.z, t.w });
		else if constexpr (std::same_as<std::decay_t<T>, texture*>)
			shad.setUniform(iter->first, texture_functions::get_sf_texture(*t));
		else
			shad.setUniform(iter->first, t);

		// set the uniform flag
		const auto dist = distance(begin(*_uniforms), iter);
		auto iter2 = std::next(begin(_uniform_set), dist);
		*iter2 = true;
		return;
	}
}

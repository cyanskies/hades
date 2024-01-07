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
			return static_cast<void>(f.template operator()<resource_link<texture>>());
		}

		throw unexpected_uniform{ "Unrecognised uniform type" };
	}

	template<typename T>
	uniform_type_list get_uniform_type_enum()
	{
		if constexpr (std::same_as<T, float>)
			return uniform_type_list::float_t;
		else if constexpr (std::same_as<T, vector2_float>)
			return uniform_type_list::vector2f;
		else if constexpr (std::same_as<T, vector3<float>>)
			return uniform_type_list::vector3f;
		else if constexpr (std::same_as<T, vector4<float>>)
			return uniform_type_list::vector4f;
		else if constexpr (std::same_as<T, int>)
			return uniform_type_list::int_t;
		else if constexpr (std::same_as<T, vector2_int>)
			return uniform_type_list::vector2i;
		else if constexpr (std::same_as<T, vector3<int>>)
			return uniform_type_list::vector3i;
		else if constexpr (std::same_as<T, vector4<int>>)
			return uniform_type_list::vector4i;
		else if constexpr (std::same_as<T, sf::Glsl::Mat3>)
			return uniform_type_list::matrix3;
		else if constexpr (std::same_as<T, sf::Glsl::Mat4>)
			return uniform_type_list::matrix4;
		else if constexpr (std::same_as<T, resource_link<texture>> ||
			std::same_as<T, sf::Shader::CurrentTextureType>)
			return uniform_type_list::texture;
		else
			return uniform_type_list::end;
	}

	template<uniform_type T>
	void shader_proxy::set_uniform(std::string_view name, T t) // needs to be part of the internal impl
	{
		assert(_shader);
		auto type = get_uniform_type_enum<std::decay_t<T>>();
		auto uni = _uniforms.find(name);
		if (uni == end(_uniforms))
		{
			throw unexpected_uniform{
				std::format("Tried to set an unexpected uniform: {}, on shader: {}",
				name,
				to_string(shader_functions::get_id(*_shader)))
			};
		}

		if (uni->second.type != type)
		{
			throw uniform_wrong_type{
					std::format("Called set_uniform with wrong type for uniform: {}, expected: {}, provided: {}",
					name, uniform_type_to_string(uni->second.type), uniform_type_to_string(type))
			};
		}

		uni->second.value = std::move(t);
		return;
	}
}

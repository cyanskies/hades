#include "hades/shader.hpp"

namespace hades::resources
{
	struct shader_t {};

	struct shader : public resource_type<sf::Shader>
	{
		//char vertex, geometry, fragment;
		shader_uniform_map uniforms;
		shader_proxy proxy;
	};

	shader_proxy::shader_proxy(shader* s, const shader_uniform_map* u)
		: _shader{ s }, _uniforms{ u }
	{
		assert(s);
		assert(u);
#ifndef NDEBUG
		_uniform_set.resize(size(*_uniforms), false);
#endif
		return;
	}

	struct uniform_visitor
	{
		shader_proxy& proxy;
		const string& name;

		void operator()(std::monostate)
		{}

		void operator()(const auto& value)
		{
			proxy.set_uniform(name, value);
			return;
		}
	};

	void shader_proxy::start_uniforms()
	{
		auto iter = begin(*_uniforms);
		const auto end = std::end(*_uniforms);

		auto iter2 = begin(_uniform_set);
		const auto end2 = std::end(_uniform_set);
		assert(size(*_uniforms) == size(_uniform_set));

		while (iter != end)
		{
			if (!std::holds_alternative<std::monostate>(iter->second.value))
			{
				std::visit(uniform_visitor{ *this, iter->first }, iter->second.value);
				*iter2 = true;
			}

			++iter;
			++iter2;
		}

		return;
	}

	void shader_proxy::end_uniforms() const
	{
		auto iter = begin(_uniform_set);
		const auto end = std::end(_uniform_set);
		while (iter != end)
		{
			if (!*iter)
			{
				const auto dist = distance(begin(_uniform_set), iter);
				auto iter2 = std::next(begin(*_uniforms), dist);
				throw uniform_not_set{ std::format("Shader cannot be used, render system didn't set uniform: {}", iter2->first) };
			}
			++iter;
		}

		return;
	}

	const sf::Shader& shader_proxy::get_shader() const
	{
		return _shader->value;
		// TODO: insert return statement here
	}
}

namespace hades::resources::shader_functions
{

}

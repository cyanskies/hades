#include "hades/debug/resource_inspector.hpp"

#include "hades/system.hpp"

namespace hades::debug
{
	template<typename ResourceInspector>
		requires is_resource_inspector<ResourceInspector>
	void enable_resource_inspector()
	{
		const auto func = [](const hades::argument_list&) {
			static ResourceInspector* inspector_overlay = {};

			if (inspector_overlay)
				inspector_overlay = hades::debug::destroy_overlay(inspector_overlay);
			else
				inspector_overlay = hades::debug::create_overlay(std::make_unique<ResourceInspector>());

			return true;
		};

		using namespace std::string_view_literals;
		console::add_function("resource_inspector"sv, func, true);
		return;
	}
}

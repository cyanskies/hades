#ifndef HADES_RESOURCE_INSPECTOR_HPP
#define HADES_RESOURCE_INSPECTOR_HPP

#include "hades/debug.hpp"

namespace hades::data
{
	class data_manager;

	class resource_editor
	{
	public:
		virtual ~resource_editor() = default;

		void set_editable_mod(unique_id m) noexcept
		{
			_editable = m;
		}

		bool editable(unique_id r) const noexcept
		{
			return _editable == r;
		}

		virtual void set_target(data_manager&, unique_id, unique_id mod) = 0;
		virtual void update(data_manager&, gui&) = 0;

	private:
		unique_id _editable = unique_zero;
	};
	
	class basic_resource_inspector
	{
	public:
		void update(gui&, data_manager&);
		void lock_editing_to(unique_id mod)
		{
			_mod = mod;
			if(_tree_state.res_editor)
				_tree_state.res_editor->set_editable_mod(_mod);
			return;
		}

		struct resource_tree_state
		{
			std::size_t mod_index = std::numeric_limits<std::size_t>::max();

			struct group_val
			{
				std::string_view res_type;
				string data_file;
				unique_id id;
				string name;
				unique_id mod_id;
				string mod;
			};

			using group_storage = std::vector<group_val>;
			using group_iter = group_storage::iterator;
			group_storage resource_groups;
			std::unique_ptr<resource_editor> res_editor;
		};

		virtual std::unique_ptr<resource_editor> make_resource_editor(std::string_view resource_type);

	private:
		void _list_resources_from_data_file(resource_tree_state::group_iter first, resource_tree_state::group_iter last, gui& g,
			data_manager&, unique_id);
		void _list_resources_from_group(resource_tree_state::group_iter first, resource_tree_state::group_iter last, gui& g,
			data_manager&, unique_id);
		void _refresh(data_manager&);
		void _resource_tree(gui&, data::data_manager&);

		resource_tree_state _tree_state;
		bool _show_by_data_file = false;
		unique_id _mod = unique_zero;
	};
}

namespace hades::debug
{
	template<typename ResourceInspector>
	concept is_resource_inspector = std::same_as<data::basic_resource_inspector, ResourceInspector>
		|| std::is_base_of_v<data::basic_resource_inspector, ResourceInspector>;

	// displays object information
	template<typename ResourceInspector = data::basic_resource_inspector>
		requires is_resource_inspector<ResourceInspector>
	class resource_inspector_overlay : public basic_overlay
	{
	public:
		void update(gui& g) override
		{
			[[maybe_unused]]
			auto [data, lock] = data::detail::get_data_manager_exclusive_lock();
			_inspector.update(g, *data);
			return;
		}

	private:	
		ResourceInspector _inspector;
	};

	template<typename ResourceInspector>
		requires is_resource_inspector<ResourceInspector>
	void enable_resource_inspector();
}

#include "hades/debug/resource_inspector.inl"

#endif // !HADES_RESOURCE_INSPECTOR_HPP

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

	using make_resource_editor_fn = std::unique_ptr<resource_editor>(*)();

	void register_resource_editor(std::string_view, make_resource_editor_fn);

	class resource_inspector
	{
	public:
		void update(gui&, data_manager*);
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
				unique_id id;
				string name;
				unique_id mod_id;
				string mod;
			};

			struct group
			{
				string name;
				std::vector<group_val> resources;
			};

			std::vector<group> resource_groups;
			std::unique_ptr<resource_editor> res_editor;
		};

	private:
		void _list_resources_from_group(resource_inspector::resource_tree_state::group& group, gui& g,
			data_manager&, unique_id);
		void _refresh(data_manager&);
		void _resource_tree(gui&, data::data_manager*);

		resource_tree_state _tree_state;
		unique_id _mod = unique_zero;
	};
}

namespace hades::debug
{
	// displays object information
	class resource_inspector_overlay : public basic_overlay
	{
	public:
		resource_inspector_overlay(data::data_manager* d)
			: _data_man{ d }
		{}

		void update(gui& g) override
		{
			_inspector.update(g, _data_man);
			return;
		}

	private:	
		data::resource_inspector _inspector;
		data::data_manager* _data_man;
	};
}

#endif // !HADES_OBJECT_OVERLAY_HPP

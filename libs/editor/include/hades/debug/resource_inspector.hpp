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
		virtual std::string resource_name() const = 0;
		virtual resources::resource_base* get() const noexcept = 0;

	private:
		unique_id _editable = unique_zero;
	};
	
	class basic_resource_inspector
	{
	public:
		basic_resource_inspector();

		void update(gui&, data_manager&);
		void lock_editing_to(unique_id mod)
		{
			_mod = mod;
			if(_tree_state.res_editor)
				_tree_state.res_editor->set_editable_mod(_mod);
			return;
		}

		bool is_resource_open() const noexcept;
		// opens a prompt to move the currently selected
		// resource to a new data file
		void prompt_new_data_file() noexcept;

		void inspect(data_manager&, unique_id, unique_id mod);
		void inspect(data_manager&, std::string_view type, unique_id, unique_id mod);
		const resources::resource_base* get_current_resource() const noexcept;

		void refresh(data_manager&);

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

		using resource_editor_function = std::function<std::unique_ptr<resource_editor>()>;

	protected:
		void register_resource_editor(std::string resource_type, resource_editor_function func)
		{
			_editor_funcs.emplace(std::move(resource_type), std::move(func));
			return;
		}
		
	private:
		struct new_data_file_prompt
		{
			bool open = false;
			std::string name = "new_data_file.yaml";
		};

		void _list_resources_from_data_file(resource_tree_state::group_iter first, resource_tree_state::group_iter last, gui& g,
			data_manager&, unique_id, vector2_float&);
		void _list_resources_from_group(resource_tree_state::group_iter first, resource_tree_state::group_iter last, gui& g,
			data_manager&, unique_id, vector2_float&);
		std::unique_ptr<resource_editor> _make_resource_editor(std::string_view resource_type);
		void _refresh(data_manager&);
		void _resource_tree(gui&, data::data_manager&);

		unordered_map_string<resource_editor_function> _editor_funcs;
		resource_tree_state _tree_state;
		new_data_file_prompt _new_datafile;
		unique_id _mod = unique_zero; 
		bool _show_by_data_file = false;
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
			auto& data = data::detail::get_data_manager();
			_inspector.update(g, data);
			return;
		}

	private:	
		ResourceInspector _inspector;
	};

	// TODO: find a way for game to open on demand and give resource id too
	template<typename ResourceInspector>
		requires is_resource_inspector<ResourceInspector>
	void enable_resource_inspector();
}

#include "hades/debug/resource_inspector.inl"

#endif // !HADES_RESOURCE_INSPECTOR_HPP

#include "hades/gui.hpp"

#include "imgui.h"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/Graphics/VertexArray.hpp"

#include "hades/data.hpp"

namespace hades
{
	gui::gui()
	{
		_activate_context();
		_generate_atlas();

		set_display_size({ 1.f, 1.f });

		frame_begin();
		frame_end();
	}

	void gui::activate_context()
	{
		_activate_context();
	}

	void gui::set_display_size(vector_t<float> size)
	{
		_active_assert();
		auto &io = ImGui::GetIO();
		io.DisplaySize = { size.x, size.y };
	}

	const gui::font *gui::add_font(const resources::font *f)
	{
		auto &io = ImGui::GetIO();
		auto &f_atlas = *io.Fonts;
		ImFontConfig cfg;
		cfg.FontDataOwnedByAtlas = false;
		//const cast, because f_atlas demands control of the ptr
		//though it won't actually do anything, since FontDataOwned is set to false
		const auto out = f_atlas.AddFontFromMemoryTTF(const_cast<std::byte*>(f->source_buffer.data()), f->source_buffer.size(), 13.f, &cfg);
		_generate_atlas();

		return out;
	}

	void gui::input_actions(time_duration dt)
	{
		//record the change in time
		_activate_context();
		auto &io = ImGui::GetIO();
		io.DeltaTime = time_cast<seconds>(dt).count();

		//set up input

		//remove captured actions from the input structure
	}

	void gui::frame_begin()
	{
		_active_assert();
		ImGui::NewFrame();
	}

	void gui::frame_end()
	{
		_active_assert();
		ImGui::EndFrame();
	}

	sf::Vertex to_vertex(ImDrawVert vert)
	{
		return { {vert.pos.x, vert.pos.y},
			sf::Color{vert.col},
			{vert.uv.x, vert.uv.y} };
	}

	void gui::draw(sf::RenderTarget & target, sf::RenderStates states) const
	{
		_active_assert();

		ImGui::Render();
		const auto draw_data = ImGui::GetDrawData();
		assert(draw_data);
		const auto first = draw_data->CmdLists;
		const auto last = draw_data->CmdLists + draw_data->CmdListsCount;

		//for each entry in the draw list
		std::for_each(first, last, [&target, states](ImDrawList *draw_list) {
			const auto *index_first = draw_list->IdxBuffer.Data;

			//for each command
			for (const auto cmd : draw_list->CmdBuffer)
			{
				if (cmd.UserCallback)
					std::invoke(cmd.UserCallback, draw_list, &cmd);
				else
				{
					//get the verts from the draw list that are associated with
					//this command
					sf::VertexArray verts{ sf::Triangles, cmd.ElemCount };
					const auto *i = index_first;
					for (decltype(ImDrawCmd::ElemCount) j = 0; j < cmd.ElemCount; ++j)
						verts[j] = to_vertex(draw_list->VtxBuffer[*(i++)]);

					if (cmd.TextureId)
					{
						const auto texture = static_cast<const resources::texture*>(cmd.TextureId);
						assert(texture);
						auto state = states;
						state.texture = &texture->value;
						target.draw(verts, state);
					}
					else
						target.draw(verts);
				}

				//move the index ptr forward for the next command
				index_first += cmd.ElemCount;
			}
		});
	}

	void gui::show_demo_window()
	{
		_active_assert();
		ImGui::ShowDemoWindow();
	}

	constexpr std::string_view gui::version()
	{
		return IMGUI_VERSION;
	}

	void gui::_activate_context()
	{
		assert(_my_context);

		ImGui::SetCurrentContext(_my_context.get());
	}

	void gui::_active_assert() const
	{
		assert(_my_context);
		assert(ImGui::GetCurrentContext() == _my_context.get());
	}

	void gui::_generate_atlas()
	{
		//get texture
		auto [d, lock] = data::detail::get_data_manager_exclusive_lock();
		std::ignore = lock;

		auto t = d->find_or_create<resources::texture>(_font_atlas_id, unique_id::zero);

		t->mips = false;
		t->repeat = false;
		t->smooth = false;

		//get the data and set the correct ids
		auto &io = ImGui::GetIO();
		auto &f_atlas = *io.Fonts;
		int width = 0, height = 0;
		unsigned char *texture_data = nullptr;
		f_atlas.GetTexDataAsRGBA32(&texture_data, &width, &height);
		f_atlas.TexID = static_cast<void*>(t);

		//make the texture
		t->value.create(width, height);
		t->value.update(texture_data);
		//apply correct settings
		t->value.setRepeated(false);
		t->value.setSmooth(false);
		t->width = width;
		t->height = height;
	}
}
#include "hades/debug.hpp"

#include <algorithm>
#include <cassert>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/console_variables.hpp"
#include "hades/data.hpp"
#include "hades/font.hpp"

//#include "hades/types.hpp"

namespace hades::debug
{
	basic_overlay* overlay_manager::create_overlay(std::unique_ptr<basic_overlay> ptr)
	{
		return _overlays.emplace_back(std::move(ptr)).get();
	}

	basic_overlay* overlay_manager::destroy_overlay(basic_overlay* ptr) noexcept
	{
		const auto iter = std::find_if(begin(_overlays), end(_overlays), [ptr](const auto& overlay) {
			return overlay.get() == ptr;
		});

		if (iter != end(_overlays))
			_overlays.erase(iter);

		return nullptr;
	}

	void overlay_manager::update(gui& g)
	{
		for (auto& elm : _overlays)
			elm->update(g);
		return;
	}

	static overlay_manager* overlay_manager_ptr = nullptr;

	void set_overlay_manager(overlay_manager* ptr) noexcept
	{
		assert(overlay_manager_ptr == nullptr);
		assert(ptr);

		overlay_manager_ptr = ptr;
		return;
	}

	basic_overlay* create_overlay(std::unique_ptr<basic_overlay> ptr)
	{
		assert(overlay_manager_ptr);
		return overlay_manager_ptr->create_overlay(std::move(ptr));
	}

	basic_overlay* destroy_overlay(basic_overlay* ptr) noexcept
	{
		assert(overlay_manager_ptr);
		return overlay_manager_ptr->destroy_overlay(ptr);
	}

	text_overlay_manager::text_overlay_manager() :
		_font{ data::get<resources::font>(resources::default_font_id()) },
		_char_size{ console::get_int(cvars::console_charsize, cvars::default_value::console_charsize) }
	{}

	text_overlay* text_overlay_manager::create_overlay(std::unique_ptr<text_overlay> overlay)
	{
		_overlays.push_back(std::move(overlay));
		return &*_overlays.back();
	}

	text_overlay* text_overlay_manager::destroy_overlay(text_overlay* ptr)
	{
		const auto loc = std::find_if(_overlays.begin(), _overlays.end(), [ptr](std::unique_ptr<text_overlay>& e) {
			return ptr == &*e;
			});

		if (loc != _overlays.end())
		{
			_overlays.erase(loc);
			return nullptr;
		}

		return ptr;
	}

	void text_overlay_manager::update()
	{
		_overlay_output.clear();
		for (auto& o : _overlays)
		{
			const auto s = o->update();
			if (!empty(s))
				_overlay_output += '\n' + s;
		}

		_display.setString(_overlay_output);
		_display.setFont(_font->value);
		_display.setCharacterSize(_char_size->load());
		return;
	}

	void text_overlay_manager::draw(time_duration, sf::RenderTarget& target, sf::RenderStates states)
	{
		target.draw(_display, states);
		return;
	}

	static text_overlay_manager* overlay_manager_old_ptr = nullptr;

	void set_text_overlay_manager(text_overlay_manager* ptr) noexcept
	{ 
		assert(overlay_manager_old_ptr == nullptr);
		assert(ptr);
		overlay_manager_old_ptr = ptr;
		return;
	}

	text_overlay* create_text_overlay(std::unique_ptr<text_overlay> overlay)
	{
		if (overlay_manager_old_ptr)
			return overlay_manager_old_ptr->create_overlay(std::move(overlay));

		return nullptr;
	}

	text_overlay* destroy_text_overlay(text_overlay* overlay)
	{
		if (overlay_manager_old_ptr)
			return overlay_manager_old_ptr->destroy_overlay(overlay);

		return overlay;
	}

	screen_overlay* screen_overlay_manager::create_overlay(std::unique_ptr<screen_overlay> overlay)
	{
		_overlays.push_back(std::move(overlay));
		return &*_overlays.back();
	}

	screen_overlay* screen_overlay_manager::destroy_overlay(screen_overlay* ptr)
	{
		const auto loc = std::find_if(_overlays.begin(), _overlays.end(), [ptr](std::unique_ptr<screen_overlay>& e) {
			return ptr == &*e;
			});

		if (loc != _overlays.end())
		{
			_overlays.erase(loc);
			return nullptr;
		}

		return ptr;
	}

	void screen_overlay_manager::draw(time_duration dt, sf::RenderTarget& target, sf::RenderStates states)
	{
		for (auto& o : _overlays)
			o->draw(dt, target, states);
		return;
	}

	static screen_overlay_manager* screen_overlay_ptr = nullptr;

	void set_screen_overlay_manager(screen_overlay_manager* ptr) noexcept
	{
		assert(screen_overlay_ptr == nullptr);
		assert(ptr);
		screen_overlay_ptr = ptr;
		return;
	}

	screen_overlay* create_screen_overlay(std::unique_ptr<screen_overlay> overlay)
	{
		if (screen_overlay_ptr)
			return screen_overlay_ptr->create_overlay(std::move(overlay));

		return nullptr;
	}

	screen_overlay* destroy_screen_overlay(screen_overlay* overlay)
	{
		if (screen_overlay_ptr)
			return screen_overlay_ptr->destroy_overlay(overlay);

		return overlay;
	}
}

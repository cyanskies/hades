#ifndef HADES_DEBUG_HPP
#define HADES_DEBUG_HPP

#include <memory>
#include <vector>

#include "SFML/Graphics/Text.hpp"
#include "SFML/System/Vector2.hpp"

#include "hades/drawable.hpp"
#include "hades/font.hpp"
#include "hades/properties.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	class gui; // defined in "hades/gui.hpp"

	namespace debug
	{
		// requires hades-gui
		class basic_overlay
		{
		public:
			virtual ~basic_overlay() noexcept = default;
			/// @brief update is called once per frame
			virtual void update(gui&) = 0;
		};

		class overlay_manager
		{
		public:
			basic_overlay* create_overlay(std::unique_ptr<basic_overlay>);
			basic_overlay* destroy_overlay(basic_overlay*) noexcept;

			void update(gui&);
		private:
			std::vector<std::unique_ptr<basic_overlay>> _new_overlays;
			std::vector<basic_overlay*> _removal_list;
			std::vector<std::unique_ptr<basic_overlay>> _overlays;
		};

		void set_overlay_manager(overlay_manager*) noexcept;

		namespace detail
		{
			basic_overlay* create_overlay(std::unique_ptr<basic_overlay>);
			basic_overlay* destroy_overlay(basic_overlay*) noexcept;
		}

		template<typename T>
		T* create_overlay(std::unique_ptr<T> o)
		{
			static_assert(std::is_base_of_v<basic_overlay, T>);
			return static_cast<T*>(detail::create_overlay(std::move(o)));
		}

		template<typename T>
		T* destroy_overlay(T* o) noexcept
		{
			static_assert(std::is_base_of_v<basic_overlay, T>);
			return static_cast<T*>(detail::destroy_overlay(o));
		}

		class text_overlay
		{
		public:
			virtual ~text_overlay() noexcept = default;
			virtual string update() = 0;
		};

		class text_overlay_manager : public drawable
		{
		public:
			text_overlay_manager();

			text_overlay* create_overlay(std::unique_ptr<text_overlay>);
			text_overlay* destroy_overlay(text_overlay*);

			void update();
			void draw(time_duration, sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates()) final override;
		private:
			std::vector<std::unique_ptr<text_overlay>> _new_overlays;
			std::vector<text_overlay*> _removal_list;
			std::vector<std::unique_ptr<text_overlay>> _overlays;
			const resources::font* _font = nullptr;
			console::property<types::int32> _char_size;
			string _overlay_output;
			sf::Text _display;
		};

		void set_text_overlay_manager(text_overlay_manager*) noexcept;
		//adds an overlay to the overlay list
		text_overlay *create_text_overlay(std::unique_ptr<text_overlay> overlay);
		//removes an overlay from the overlay list
		//sets the passed overlay ptr to nullptr if successful
		text_overlay* destroy_text_overlay(text_overlay*);

		class screen_overlay : public drawable
		{};

		class screen_overlay_manager : public drawable
		{
		public:
			screen_overlay* create_overlay(std::unique_ptr<screen_overlay>);
			screen_overlay* destroy_overlay(screen_overlay*) noexcept;

			void draw(time_duration, sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates()) final override;
		private:
			std::vector<std::unique_ptr<screen_overlay>> _overlays;
		};

		void set_screen_overlay_manager(screen_overlay_manager*) noexcept;
		namespace detail
		{
			screen_overlay* create_screen_overlay(std::unique_ptr<screen_overlay> overlay) noexcept;
			screen_overlay* destroy_screen_overlay(screen_overlay*) noexcept;
		}

		//adds an overlay to the overlay list
		template<typename T>
		T* create_screen_overlay(std::unique_ptr<T> overlay) noexcept
		{
			static_assert(std::is_base_of_v<screen_overlay, T>);
			return static_cast<T*>(detail::create_screen_overlay(std::move(overlay)));
		}
		//removes an overlay from the overlay list
		//sets the passed overlay ptr to nullptr if successful
		template<typename T>
		T* destroy_screen_overlay(T* overlay) noexcept
		{
			static_assert(std::is_base_of_v<screen_overlay, T>);
			return static_cast<T*>(detail::destroy_screen_overlay(overlay));
		}
	}
}

#endif //HADES_DEBUG_HPP

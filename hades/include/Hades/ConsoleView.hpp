#ifndef HADES_CONSOLEVIEW_HPP
#define HADES_CONSOLEVIEW_HPP

#include <vector>

#include "SFML/Window/Event.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/View.hpp"

#include "Hades/Debug.hpp"
#include "hades/font.hpp"
#include "Hades/Logging.hpp"
#include "Hades/Properties.hpp"
#include "Hades/Types.hpp"

namespace hades
{
	constexpr auto console_character_size = "con_charactersize";
	constexpr auto console_fade = "con_fade";
	constexpr auto console_character_size_d = 15;
	constexpr auto console_fade_d = 180;

	class ConsoleWrongEvent : public std::logic_error
	{
	public:
		using std::logic_error::logic_error;
	};

	class ConsoleView : public debug::Overlay
	{
	public:
		ConsoleView();

		void setFullscreenSize(sf::Vector2f) override;
		void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates()) const override;

		void update();
		void enterText(const sf::Event &context);
		void prev();
		void next();
		void sendCommand();

	private:
		void _reinit(sf::Vector2f);
		void _addText(const console::string &s);

		sf::View _view;
		sf::View _textView;

		const resources::font *_font = nullptr;
		std::vector<sf::Text> _previousOutput;
		sf::Text _currentInput;
		sf::RectangleShape _backdrop, _editLine;

		console::property<types::int32> _charSize, _fade;
		types::string _input;
	};
}

#endif //HADES_CONSOLEVIEW_HPP

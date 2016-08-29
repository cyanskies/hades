#ifndef HADES_CONSOLEVIEW_HPP
#define HADES_CONSOLEVIEW_HPP

#include <string>
#include <vector>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Transformable.hpp"
#include "SFML/Graphics/View.hpp"

#include "Hades/Bind.hpp"
#include "Hades/Console.hpp"
#include "Stitches.hpp"

namespace hades
{
	class ConsoleView : public sf::Drawable, public hades::Common_Uses
	{
	public:
		ConsoleView();

		virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates()) const;

		using hades::Common_Uses::attach;

		void init();

		void update();
		bool active() const;
		bool toggleActive();
		void enterText(hades::EventContext context);
		void sendCommand();

	private:
		
		float _currentYPos;
		sf::View _view;
		std::shared_ptr<const sf::Font> _font;
		hades::ConsoleVariable<int> _charSize, _screenW, _screenH, _consoleFade;
		std::string _inputText;
		bool _active;

		typedef std::vector<sf::Text> StringLinesList;
		StringLinesList _visibleOutput;
		sf::Transformable _transform;
		sf::Text _currentInput;
		sf::RectangleShape _backdrop, _editLine;
	};
}

#endif //HADES_CONSOLEVIEW_HPP
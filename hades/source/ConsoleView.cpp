#include "Hades/ConsoleView.hpp"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/System/String.hpp"

#include "Hades/Properties.hpp"
#include "Hades/resource/fonts.hpp"
#include "Hades/System.hpp"

const float SCREEN_LEFT = 0.f;
const std::string INPUT_SYMBOL = ":>";

hades::ConsoleView::ConsoleView() : _active(false), _currentYPos(0.f) 
{}

void hades::ConsoleView::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	//render faded backdrop
	auto view = target.getDefaultView();

	//_view.move(_transform.getPosition());

	target.setView(_view);
	//apply local translation so that the text appears to scroll

	target.draw(_backdrop, states);

	for (auto &t : _visibleOutput)
		target.draw(t, states);

	target.draw(_currentInput, states);
	target.draw(_editLine, states);
}

void hades::ConsoleView::init()
{
	//_font = resource->getResource<sf::Font>(console->getValue<std::string>("con_characterfont")->load()); // this doesn't work, atomic<std::string is illigal.
	_font.loadFromMemory(console_font::data, console_font::length);
	_charSize = console::getInt("con_charactersize", 15);
	_screenW = console::getInt("vid_width", 800);
	_screenH = console::getInt("vid_height", 600);
	_consoleFade = console::getInt("con_fade", 180);
	
	_view.reset(sf::FloatRect(0.f, 0.f, static_cast<float>(*_screenW), static_cast<float>(*_screenH)));

	_currentInput.setFont(_font);
	_editLine.setFillColor(sf::Color::White);
	_editLine.setOutlineColor(sf::Color::White);

	assert(_charSize);
}

void hades::ConsoleView::update()
{
	//TODO: support infolevel filters.
	auto strings = console::new_output(Console::Console_String_Verbosity::WARNING);

	for (auto s : strings)
	{
		_visibleOutput.emplace_back(s.Text(), _font, *_charSize);
		_visibleOutput.back().setPosition(SCREEN_LEFT, _currentYPos);

		sf::Color col = sf::Color::White;
		if (s.Verbosity() == Console::Console_String_Verbosity::WARNING)
			col = sf::Color::Yellow;
		else if (s.Verbosity() == Console::Console_String_Verbosity::ERROR)
			col = sf::Color::Red;

		_visibleOutput.back().setFillColor(col);
		_visibleOutput.back().setOutlineColor(col);

		_currentYPos -= *_charSize; // may have to add a charBuffer(con_charbuffer, or con_linespacing) so that each line has a gap between;
	}

	//recalculate transform offset
	_backdrop.setPosition(SCREEN_LEFT, _currentYPos - *_charSize);
	_currentInput.setPosition(SCREEN_LEFT, _currentYPos - *_charSize);
	_currentInput.setCharacterSize(*_charSize);
	_currentInput.setString(INPUT_SYMBOL + _inputText);

	//place the seperating line between the edit box and the output
	_editLine.setPosition(SCREEN_LEFT, _currentYPos);

	//recalculate view position
	_transform.setPosition(SCREEN_LEFT, _currentYPos - *_charSize); // - editbox height
	_view.reset(sf::FloatRect(0.f, 0.f, static_cast<float>(*_screenW), static_cast<float>(*_screenH)));
	_view.move(_transform.getPosition());
}

bool hades::ConsoleView::active() const
{
	return _active;
}

bool hades::ConsoleView::toggleActive()
{
	_active = !_active;

	if (_active)
	{
		_backdrop.setFillColor(sf::Color(0, 0, 0, *_consoleFade));
		_backdrop.setSize(sf::Vector2f(static_cast<float>(*_screenW), static_cast<float>(*_screenH)));
		_editLine.setSize(sf::Vector2f(static_cast<float>(*_screenW), 5.f));
	}

	return _active;
}

void hades::ConsoleView::enterText(hades::EventContext context)
{
	auto text = sf::String(context.event->text.unicode).toAnsiString();

	if (text == "\b")
	{
		if (!_inputText.empty())
			_inputText.pop_back();
	}
	//don't add special control characters
	else if(text != "\n" && text != "\r" 
		&& text != "`")
		_inputText += text;
}

void hades::ConsoleView::sendCommand()
{
	console::runCommand(_inputText);

	_inputText.clear();
}
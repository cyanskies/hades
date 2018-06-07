#include "Hades/ConsoleView.hpp"

#include <algorithm>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/System/String.hpp"

#include "Hades/resource/fonts.hpp"
#include "Hades/System.hpp"
#include "Hades/Utility.hpp"

constexpr float SCREEN_LEFT = 0.f;
constexpr auto INPUT_SYMBOL = ":>";

namespace hades
{
	ConsoleView::ConsoleView() : Overlay(true)
	{
		_font.loadFromMemory(console_font::data, console_font::length);
		_fade = console::get_int("con_fade", 180);
		_charSize = console::get_int("con_charactersize", 15);
	}

	void ConsoleView::setFullscreenSize(sf::Vector2f size)
	{
		_reinit(size);
	}

	void ConsoleView::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		target.setView(_view);
		target.draw(_backdrop);

		target.setView(_textView);
		for (auto &t : _previousOutput)
			target.draw(t);

		target.setView(_view);
		target.draw(_editLine);
		target.draw(_currentInput);
	}

	sf::View setTextView(const sf::Text &last, sf::Vector2f size, float lineHeight, float extraHeight)
	{
		return sf::View({ 0.f, last.getGlobalBounds().top - size.y +
			last.getGlobalBounds().height + lineHeight * 2 - extraHeight, size.x, size.y });
	}

	unsigned int ValidCharSize(hades::console::property_int char_size)
	{
		const auto n = char_size->load();
		if (n < 0)
			char_size->store(0);

		return static_cast<unsigned int>(n);
	}

	void ConsoleView::update()
	{
		const auto log_list = console::new_output(console::logger::log_verbosity::warning);

		for (const auto &s : log_list)
			_addText(s);

		_currentInput.setString(INPUT_SYMBOL + _input);

		_textView = setTextView(_previousOutput.back(), _view.getSize(), static_cast<float>(ValidCharSize(_charSize)), _editLine.getSize().y);
	}

	void ConsoleView::enterText(const sf::Event &context)
	{
		if (context.type != sf::Event::TextEntered)
			throw ConsoleWrongEvent("Passed an event other than sf::Event:TextEntered to the console view overlay.");

		const auto text = sf::String(context.text.unicode).toAnsiString();

		if (text == "\b")
		{
			if (!_input.empty())
				_input.pop_back();
		}
		//'\033' is the octal code for ESC; clear the input
		else if (text == "\033")
			_input.clear();
		//don't add special control characters
		else if (text != "\n" && text != "\r"
			&& text != "`")
			_input += text;
	}

	void ConsoleView::prev()
	{
		const auto prev = console::command_history();

		if (prev.empty())
			return;

		const auto pos = std::find(prev.begin(), prev.end(), command(_input));
		if (pos == prev.end())
			_input = to_string(prev.back());
		else if (pos != prev.begin())
			_input = to_string(*(pos - 1));
		else
			_input = to_string(*pos);
	}

	void ConsoleView::next()
	{
		const auto prev = console::command_history();

		if (prev.empty() || _input.empty())
			return;

		const auto pos = std::find(prev.begin(), prev.end(), command(_input));
		if (pos == prev.end())
			_input = to_string(prev.front());
		else if (pos != --prev.end())
			_input = to_string(*(pos + 1));
		else
			_input = to_string(*pos);
	}

	void ConsoleView::sendCommand()
	{
		console::run_command(command(_input));
		_input.clear();
	}

	void ConsoleView::_reinit(sf::Vector2f size)
	{
		_backdrop.setFillColor(sf::Color(0, 0, 0, *_fade));
		_backdrop.setSize(sf::Vector2f(size.x, size.y));
		_editLine.setSize(sf::Vector2f(size.x, 5.f));

		const auto char_size = ValidCharSize(_charSize);

		_currentInput.setCharacterSize(char_size);
		_currentInput.setFont(_font);

		const auto offset = char_size;
		_editLine.setPosition(0.f, size.y - _editLine.getSize().y - offset);
		_currentInput.setPosition(0.f, size.y - offset);

		_view = sf::View({ 0.f, 0.f, size.x, size.y });

		_previousOutput.clear();

		const auto output = console::output(console::logger::log_verbosity::warning);
		for (auto &s : output)
			_addText(s);

		_textView = setTextView(_previousOutput.back(), size, static_cast<float>(offset), _editLine.getSize().y);
	}

	float GetTextHeight(const std::vector<sf::Text> &text)
	{
		float height = 0.f;

		for (auto &t : text)
			height += t.getGlobalBounds().height;

		return height;
	}

	void ConsoleView::_addText(const console::string &s)
	{
		float height = 0.f;

		if(!_previousOutput.empty())
			height = _previousOutput.back().getGlobalBounds().top +
			_previousOutput.back().getGlobalBounds().height;

		std::vector<types::string> words;
		split(s.text(), ' ', std::back_inserter(words));

		sf::Color col = sf::Color::White;
		if (s.verbosity() == console::logger::log_verbosity::warning)
			col = sf::Color::Yellow;
		else if (s.verbosity() == console::logger::log_verbosity::error)
			col = sf::Color::Red;

		const auto char_size = ValidCharSize(_charSize);

		std::vector<sf::Text> output;
		output.push_back({ sf::String{}, _font, char_size });
		auto &text = output.back();
		text.setPosition({ 0.f, height });
		text.setOutlineColor(col);
		text.setFillColor(col);

		const auto max_width = _view.getSize().x;

		for (const auto &s : words)
		{
			const auto str = s + ' ';
			const auto current_line = output.back().getString();
			const sf::Text test_text{ current_line + str, _font, char_size };
			const auto bounds = test_text.getGlobalBounds();
			if (bounds.width > max_width)
			{
				const auto bounds = output.back().getGlobalBounds();
				output.push_back({ '\t' + s + ' ', _font, char_size });
				auto &t = output.back();
				t.setPosition({ 0.f, bounds.top + bounds.height });
				t.setOutlineColor(col);
				t.setFillColor(col);
			}
			else
				output.back().setString(current_line + str);
		}

		for (auto &t : output)
			_previousOutput.push_back(t);
	}
}

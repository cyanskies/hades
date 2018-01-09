#include "Hades/ConsoleView.hpp"

#include <algorithm>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/System/String.hpp"

#include "Hades/resource/fonts.hpp"
#include "Hades/System.hpp"
#include "Hades/Utility.hpp"

const float SCREEN_LEFT = 0.f;
const auto INPUT_SYMBOL = ":> ";

namespace hades
{
	ConsoleView::ConsoleView() : Overlay(true)
	{
		_font.loadFromMemory(console_font::data, console_font::length);
		_fade = console::GetInt("con_fade", 180);
		_charSize = console::GetInt("con_charactersize", 15);
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
			last.getGlobalBounds().height + lineHeight * 2 + extraHeight, size.x, size.y });
	}

	void ConsoleView::update()
	{
		auto log_list = console::new_output(console::logger::LOG_VERBOSITY::WARNING);

		for (auto &s : log_list)
			_addText(s);

		_currentInput.setString(INPUT_SYMBOL + _input);

		_textView = setTextView(_previousOutput.back(), _view.getSize(), _currentInput.getGlobalBounds().height, _editLine.getSize().y);
	}

	void ConsoleView::enterText(const sf::Event &context)
	{
		if (context.type != sf::Event::TextEntered)
			throw ConsoleWrongEvent("Passed an event other than sf::Event:TextEntered to the console view overlay.");

		auto text = sf::String(context.text.unicode).toAnsiString();

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
		auto prev = console::GetCommandHistory();

		if (prev.empty())
			return;

		auto pos = std::find(prev.begin(), prev.end(), Command(_input));
		if (pos == prev.end())
			_input = to_string(prev.back());
		else if (pos != prev.begin())
			_input = to_string(*--pos);
		else
			_input = to_string(*pos);
	}

	void ConsoleView::next()
	{
		auto prev = console::GetCommandHistory();

		if (prev.empty() || _input.empty())
			return;

		auto pos = std::find(prev.begin(), prev.end(), Command(_input));
		if (pos == prev.end())
			_input = to_string(prev.front());
		else if (pos != --prev.end())
			_input = to_string(*++pos);
		else
			_input = to_string(*pos);
	}

	void ConsoleView::sendCommand()
	{
		console::RunCommand(Command(_input));
		_input.clear();
	}

	void ConsoleView::_reinit(sf::Vector2f size)
	{
		_backdrop.setFillColor(sf::Color(0, 0, 0, *_fade));
		_backdrop.setSize(sf::Vector2f(size.x, size.y));
		_editLine.setSize(sf::Vector2f(size.x, 5.f));

		_currentInput.setCharacterSize(*_charSize);
		_currentInput.setFont(_font);

		auto size_test = _currentInput;
		size_test.setString("a");

		auto offset = size_test.getGlobalBounds().height * 2;
		_editLine.setPosition(0.f, size.y - _editLine.getSize().y - offset);
		_currentInput.setPosition(0.f, size.y - offset);

		_view = sf::View({ 0.f, 0.f, size.x, size.y });

		_previousOutput.clear();

		auto output = console::output(console::logger::LOG_VERBOSITY::WARNING);
		for (auto &s : output)
			_addText(s);

		_textView = setTextView(_previousOutput.back(), size, size_test.getGlobalBounds().height, _editLine.getSize().y);
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
		split(s.Text(), ' ', std::back_inserter(words));

		sf::Color col = sf::Color::White;
		if (s.Verbosity() == console::logger::LOG_VERBOSITY::WARNING)
			col = sf::Color::Yellow;
		else if (s.Verbosity() == console::logger::LOG_VERBOSITY::ERROR)
			col = sf::Color::Red;

		std::vector<sf::Text> output;
		output.push_back({ "", _font, static_cast<unsigned int>(*_charSize) });
		auto &text = output.back();
		text.setPosition({ 0.f, height });
		text.setOutlineColor(col);
		text.setFillColor(col);

		auto max_width = _view.getSize().x;

		for (auto &s : words)
		{
			auto str = output.back().getString();
			str += s + " ";

			auto test_text = output.back();
			test_text.setString(str);
			auto bounds = test_text.getGlobalBounds();
			if (bounds.width > max_width)
			{
				output.push_back({ "", _font, static_cast<unsigned int>(*_charSize) });
				auto &t = output.back();
				t.setPosition({ 0.f, bounds.top + bounds.height});
				t.setOutlineColor(col);
				t.setFillColor(col);
				t.setString("\t" + s + " ");
			}
			else
				output.back() = test_text;
		}

		for (auto &t : output)
			_previousOutput.push_back(t);
	}
}

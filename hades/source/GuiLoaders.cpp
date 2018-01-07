#include "Hades/GuiLoaders.hpp"

#include <memory>
#include <string>

#include "TGUI/Font.hpp"
#include "TGUI/Texture.hpp"

#include "TGUI/Loading/Deserializer.hpp"

#include "Hades/Data.hpp"
#include "Hades/simple_resources.hpp"

namespace hades
{
	std::unique_ptr<sf::Image> ImgLoader(const sf::String&);
	tgui::ObjectConverter FontLoader(const std::string&);

	void ReplaceTGuiLoaders()
	{
		//img loader handles retriving images from files
		tgui::Texture::setImageLoader(ImgLoader);
		//font deserializer handles
		tgui::Deserializer::setFunction(tgui::ObjectConverter::Type::Font, FontLoader);
	}

	std::unique_ptr<sf::Image> ImgLoader(const sf::String& name)
	{
		auto id = data::GetUid(static_cast<std::string>(name));
		assert(data::Exists(id));

		//TODO:
		//data_manager->refresh(id);
		//data_manager->load(id);

		auto img = data::Get<resources::texture>(id);
		return std::make_unique<sf::Image>(img->value.copyToImage());
	}

	tgui::ObjectConverter FontLoader(const std::string& name)
	{
		auto id = data::GetUid(name);
		assert(data::Exists(id));

		//data_manager->refresh(id);
		//data_manager->load(id);

		auto font = data::Get<resources::font>(id);

		return std::make_shared<sf::Font>(font->value);
	}
}
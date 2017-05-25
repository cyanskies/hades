#include "Hades/GuiLoaders.hpp"

#include <memory>
#include <string>

#include "TGUI/Font.hpp"
#include "TGUI/Texture.hpp"

#include "TGUI/Loading/Deserializer.hpp"

#include"Hades/DataManager.hpp"

namespace hades
{
	std::shared_ptr<sf::Image> ImgLoader(const sf::String&);
	tgui::ObjectConverter FontLoader(const std::string&);

	void ReplaceTGuiLoaders()
	{
		//img loader handles retriving images from files
		tgui::Texture::setImageLoader(ImgLoader);
		//font deserializer handles
		tgui::Deserializer::setFunction(tgui::ObjectConverter::Type::Font, FontLoader);
	}

	//TODO: these only provide copies of the resources, not access to the managed ones.
	std::shared_ptr<sf::Image> ImgLoader(const sf::String& name)
	{
		auto id = data_manager->getUid(name);
		assert(data_manager->exists(id));

		data_manager->refresh(id);
		data_manager->load(id);

		auto img = data_manager->getTexture(id);
		return std::make_shared<sf::Image>(img->value.copyToImage());
	}

	tgui::ObjectConverter FontLoader(const std::string& name)
	{
		auto id = data_manager->getUid(name);
		assert(data_manager->exists(id));

		data_manager->refresh(id);
		data_manager->load(id);

		auto font = data_manager->getFont(id);

		return std::make_shared<sf::Font>(font->value);
	}
}
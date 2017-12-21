#include "Gui/Gui.hpp"

#include "TGUI/Loading/WidgetFactory.hpp"

#include "Gui/SimpleHorizontalLayout.hpp"
#include "Gui/SimpleVerticalLayout.hpp"

namespace hades
{
	namespace gui
	{
		void RegisterGuiElements()
		{
			tgui::WidgetFactory::setConstructFunction("SimpleHorizontalLayout", std::make_shared<SimpleHorizontalLayout>);
			tgui::WidgetFactory::setConstructFunction("SimpleVerticalLayout", std::make_shared<SimpleVerticalLayout>);
		}
	}
}
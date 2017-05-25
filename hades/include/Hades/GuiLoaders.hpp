#ifndef HADES_GUI_LOADERS_HPP
#define HADES_GUI_LOADERS_HPP

namespace hades
{

	//hooks the tgui resource loaders to use the data manager instead of the default file loaders
	void ReplaceTGuiLoaders();
}

#endif // hades_gui_loaders_hpp

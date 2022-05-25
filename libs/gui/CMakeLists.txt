include(../../cmake.txt)

set(HADES_GUI_SRC
	source/gui.cpp
	include/hades/gui.hpp	
	include/hades/detail/gui.inl
)

set(HADES_GUI_LIBS 
	hades-core
	SFML::Graphics
	imgui)

hades_make_library(hades-gui include "${HADES_GUI_SRC}" "${HADES_GUI_LIBS}")

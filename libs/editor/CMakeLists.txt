include(../../cmake.txt)

set(HADES_EDITOR_SRC
	source/mod_editor.cpp
	include/hades/mod_editor.hpp
)

set(HADES_EDITOR_LIBS 
	hades-util 
	hades-basic
	hades-core
	imgui
	PRIVATE	minizip)

hades_make_library(hades-editor include "${HADES_EDITOR_SRC}" "${HADES_EDITOR_LIBS}")

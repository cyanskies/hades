include(../../cmake.txt)

set(HADES_EDITOR_SRC
	source/level_editor.cpp
	source/level_editor_grid.cpp
	source/level_editor_level_properties.cpp
	source/level_editor_objects.cpp
	source/level_editor_regions.cpp
	source/level_editor_terrain.cpp
	source/level_editor_tiles.cpp
	source/mission_editor.cpp
	source/mod_editor.cpp
	include/hades/level_editor.hpp
	include/hades/level_editor_component.hpp
	include/hades/level_editor_grid.hpp
	include/hades/level_editor_level_properties.hpp
	include/hades/level_editor_objects.hpp
	include/hades/level_editor_regions.hpp
	include/hades/level_editor_terrain.hpp
	include/hades/level_editor_tiles.hpp
	include/hades/mission_editor.hpp
	include/hades/mod_editor.hpp
	include/hades/detail/level_editor.inl
	include/hades/detail/level_editor_grid.inl
	include/hades/detail/level_editor_objects.inl
)

set(HADES_EDITOR_LIBS 
	hades-util 
	hades-basic
	hades-core
	hades-gui
)

hades_make_library(hades-editor include "${HADES_EDITOR_SRC}" "${HADES_EDITOR_LIBS}")

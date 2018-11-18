# Directory hades/source

# C++ source files
set(HADES_CORE_SRC
	animation.cpp
	archive.cpp
	camera.cpp
	core_curves.cpp
	core_resources.cpp
	files.cpp
	font.cpp
	game_system.cpp
	gui.cpp
	level.cpp
	level_editor.cpp
	level_editor_level_properties.cpp
	level_editor_objects.cpp
	level_interface.cpp
	mouse_input.cpp
	objects.cpp
	sf_input.cpp
	sprite_batch.cpp
	standard_paths.cpp
	state.cpp
	texture.cpp
	../include/hades/animation.hpp
	../include/hades/archive.hpp
	../include/hades/camera.hpp
	../include/hades/core_curves.hpp
	../include/hades/core_resources.hpp
	../include/hades/files.hpp
	../include/hades/font.hpp
	../include/hades/game_system.hpp
	../include/hades/gui.hpp
	../include/hades/level.hpp
	../include/hades/level_editor.hpp
	../include/hades/level_editor_component.hpp
	../include/hades/level_editor_grid.hpp
	../include/hades/level_editor_level_properties.hpp
	../include/hades/level_editor_objects.hpp
	../include/hades/level_interface.hpp
	../include/hades/mouse_input.hpp
	../include/hades/objects.hpp
	../include/hades/sf_input.hpp
	../include/hades/sprite_batch.hpp
	../include/hades/standard_paths.hpp
	../include/hades/state.hpp
	../include/hades/texture.hpp
	../include/hades/time.hpp
	../include/hades/detail/game_system.inl
	../include/hades/detail/gui.inl
	../include/hades/detail/level_editor.inl
	../include/hades/detail/level_interface.inl
	../include/hades/detail/mouse_input.inl
	../include/hades/detail/state.inl
)

source_group("Headers" FILES
	../include/hades/animation.hpp
	../include/hades/archive.hpp
	../include/hades/camera.hpp
	../include/hades/core_curves.hpp
	../include/hades/core_resources.hpp
	../include/hades/files.hpp
	../include/hades/font.hpp
	../include/hades/game_system.hpp
	../include/hades/gui.hpp
	../include/hades/level.hpp
	../include/hades/level_editor.hpp
	../include/hades/level_editor_component.hpp
	../include/hades/level_editor_grid.hpp
	../include/hades/level_editor_level_properties.hpp
	../include/hades/level_editor_objects.hpp
	../include/hades/level_interface.hpp
	../include/hades/mouse_input.hpp
	../include/hades/objects.hpp
	../include/hades/sf_input.hpp
	../include/hades/sprite_batch.hpp
	../include/hades/standard_paths.hpp
	../include/hades/state.hpp
	../include/hades/texture.hpp
	../include/hades/time.hpp
)

source_group("Detail" FILES
	../include/hades/detail/game_system.inl
	../include/hades/detail/gui.inl
	../include/hades/detail/level_editor.inl
	../include/hades/detail/level_interface.inl
	../include/hades/detail/mouse_input.inl
	../include/hades/detail/state.inl
)

# Library to build
set(HADES_CORE_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${HADES_CORE_LIB} STATIC ${HADES_CORE_SRC})
set_target_properties(${HADES_CORE_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${HADES_CORE_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${HADES_CORE_LIB} PUBLIC ../include)

target_link_libraries(${HADES_CORE_LIB} PUBLIC 
	hades-util
	hades-basic 
	imgui
	sfml-audio
	sfml-graphics
	sfml-main
	sfml-network
	sfml-system
	sfml-window
	minizip
)

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${HADES_CORE_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)

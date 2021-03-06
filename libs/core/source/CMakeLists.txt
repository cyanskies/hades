# C++ source files
set(HADES_CORE_SRC
	animation.cpp
	archive.cpp
	background.cpp
	camera.cpp
	console_functions.cpp
	core_curves.cpp
	core_resources.cpp
	draw_clamp.cpp
	export_curves.cpp
	files.cpp
	font.cpp
	game_api.cpp
	game_instance.cpp
	game_system.cpp
	grid.cpp
	gui.cpp
	level.cpp
	level_editor.cpp
	level_editor_grid.cpp
	level_editor_level_properties.cpp
	level_editor_objects.cpp
	level_editor_regions.cpp
	level_editor_terrain.cpp
	level_editor_tiles.cpp
	level_interface.cpp
	level_scripts.cpp
	mission.cpp
	mission_editor.cpp
	mouse_input.cpp
	objects.cpp
	render_instance.cpp
	render_interface.cpp
	sf_input.cpp
	sf_streams.cpp
	sprite_batch.cpp
	standard_paths.cpp
	state.cpp
	terrain_map.cpp
	texture.cpp
	tiled_sprite.cpp
	tile_map.cpp
	vertex_buffer.cpp
	../include/hades/animation.hpp
	../include/hades/archive.hpp
	../include/hades/background.hpp
	../include/hades/camera.hpp
	../include/hades/console_functions.hpp
	../include/hades/core_curves.hpp
	../include/hades/core_resources.hpp
	../include/hades/drawable.hpp
	../include/hades/draw_clamp.hpp
	../include/hades/export_curves.hpp
	../include/hades/files.hpp
	../include/hades/font.hpp
	../include/hades/game_api.hpp
	../include/hades/game_instance.hpp
	../include/hades/game_system.hpp
	../include/hades/grid.hpp
	../include/hades/gui.hpp
	../include/hades/level.hpp
	../include/hades/level_curve_data.hpp
	../include/hades/level_editor.hpp
	../include/hades/level_editor_component.hpp
	../include/hades/level_editor_grid.hpp
	../include/hades/level_editor_level_properties.hpp
	../include/hades/level_editor_objects.hpp
	../include/hades/level_editor_regions.hpp
	../include/hades/level_editor_terrain.hpp
	../include/hades/level_editor_tiles.hpp
	../include/hades/level_interface.hpp
	../include/hades/level_scripts.hpp
	../include/hades/mission.hpp
	../include/hades/mission_editor.hpp
	../include/hades/mouse_input.hpp
	../include/hades/objects.hpp
	../include/hades/players.hpp
	../include/hades/render_instance.hpp
	../include/hades/render_interface.hpp
	../include/hades/sf_input.hpp
	../include/hades/sf_streams.hpp
	../include/hades/sf_time.hpp
	../include/hades/sprite_batch.hpp
	../include/hades/standard_paths.hpp
	../include/hades/state.hpp
	../include/hades/terrain_map.hpp
	../include/hades/texture.hpp
	../include/hades/tiled_sprite.hpp
	../include/hades/tile_map.hpp
	../include/hades/vertex_buffer.hpp
	../include/hades/detail/game_api.inl
	../include/hades/detail/game_system.inl
	../include/hades/detail/gui.inl
	../include/hades/detail/level_curve_data.inl
	../include/hades/detail/level_editor.inl
	../include/hades/detail/level_editor_grid.inl
	../include/hades/detail/level_interface.inl
	../include/hades/detail/level_editor_objects.inl
	../include/hades/detail/mouse_input.inl
	../include/hades/detail/state.inl
)

source_group("Headers" FILES
	../include/hades/animation.hpp
	../include/hades/archive.hpp
	../include/hades/background.hpp
	../include/hades/camera.hpp
	../include/hades/console_functions.hpp
	../include/hades/core_curves.hpp
	../include/hades/core_resources.hpp
	../include/hades/drawable.hpp
	../include/hades/draw_clamp.hpp
	../include/hades/export_curves.hpp
	../include/hades/files.hpp
	../include/hades/font.hpp
	../include/hades/game_instance.hpp
	../include/hades/game_system.hpp
	../include/hades/grid.hpp
	../include/hades/gui.hpp
	../include/hades/level.hpp
	../include/hades/level_curve_data.hpp
	../include/hades/level_editor.hpp
	../include/hades/level_editor_component.hpp
	../include/hades/level_editor_grid.hpp
	../include/hades/level_editor_level_properties.hpp
	../include/hades/level_editor_objects.hpp
	../include/hades/level_editor_regions.hpp
	../include/hades/level_editor_terrain.hpp
	../include/hades/level_editor_tiles.hpp
	../include/hades/level_interface.hpp
	../include/hades/mouse_input.hpp
	../include/hades/objects.hpp
	../include/hades/render_instance.hpp
	../include/hades/render_interface.hpp
	../include/hades/sf_input.hpp
	../include/hades/sf_streams.hpp
	../include/hades/sf_time.hpp
	../include/hades/sprite_batch.hpp
	../include/hades/standard_paths.hpp
	../include/hades/state.hpp
	../include/hades/terrain_map.hpp
	../include/hades/texture.hpp
	../include/hades/tiled_sprite.hpp
	../include/hades/tile_map.hpp
	../include/hades/vertex_buffer.hpp
)

source_group("Detail" FILES
	../include/hades/detail/game_system.inl
	../include/hades/detail/gui.inl
	../include/hades/detail/level_curve_data.inl
	../include/hades/detail/level_editor.inl
	../include/hades/detail/level_editor_grid.inl
	../include/hades/detail/level_interface.inl
	../include/hades/detail/level_editor_objects.inl
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

include(../../cmake.txt)

set(HADES_CORE_SRC
	source/animation.cpp
	source/background.cpp
	source/camera.cpp
	source/console_functions.cpp
	source/core_curves.cpp
	source/core_resources.cpp
	source/debug.cpp
	source/draw_clamp.cpp
	source/export_curves.cpp
	source/font.cpp
	source/game_api.cpp
	source/game_state.cpp
	source/game_system.cpp
	source/grid.cpp
	source/level.cpp
	source/level_interface.cpp
	source/level_scripts.cpp
	source/mission.cpp
	source/mouse_input.cpp
	source/objects.cpp
	source/render_instance.cpp
	source/render_interface.cpp
	source/save_load_api.cpp
	source/sf_input.cpp
	source/shader.cpp
	source/sprite_batch.cpp
	source/state.cpp
	source/terrain_map.cpp
	source/texture.cpp
	source/tiled_sprite.cpp
	source/tile_map.cpp
	source/vertex_buffer.cpp
	source/debug/object_overlay.cpp
	include/hades/animation.hpp
	include/hades/background.hpp
	include/hades/camera.hpp
	include/hades/console_functions.hpp
	include/hades/core_curves.hpp
	include/hades/core_resources.hpp
	include/hades/debug.hpp
	include/hades/drawable.hpp
	include/hades/draw_clamp.hpp
	include/hades/export_curves.hpp
	include/hades/font.hpp
	include/hades/game_api.hpp
	include/hades/game_state.hpp
	include/hades/game_system.hpp
	include/hades/game_system_resources.hpp
	include/hades/grid.hpp
	include/hades/level.hpp
	include/hades/level_interface.hpp
	include/hades/level_scripts.hpp
	include/hades/mission.hpp
	include/hades/mouse_input.hpp
	include/hades/objects.hpp
	include/hades/players.hpp
	include/hades/render_instance.hpp
	include/hades/render_interface.hpp
	include/hades/save_load_api.hpp
	include/hades/sf_color.hpp
	include/hades/sf_input.hpp
	include/hades/sf_streams.hpp
	include/hades/sf_time.hpp
	include/hades/shader.hpp
	include/hades/sprite_batch.hpp
	include/hades/state.hpp
	include/hades/terrain_map.hpp
	include/hades/texture.hpp
	include/hades/tiled_sprite.hpp
	include/hades/tile_map.hpp
	include/hades/vertex_buffer.hpp
	include/hades/debug/object_overlay.hpp
	include/hades/detail/game_api.inl
	include/hades/detail/game_state.inl
	include/hades/detail/game_system.inl
	include/hades/detail/level_interface.inl
	include/hades/detail/mouse_input.inl
	include/hades/detail/shader.inl
	include/hades/detail/state.inl
)

set(HADES_CORE_LIBS 
	hades-util 
	hades-basic
	sfml-graphics
	plf-colony
)

hades_make_library(hades-core include "${HADES_CORE_SRC}" "${HADES_CORE_LIBS}")

source_group("Header Files\\debug" REGULAR_EXPRESSION
	"include\/hades\/debug\/.+\.hpp"
)

source_group("Source Files\\debug" REGULAR_EXPRESSION
	"source\/debug\/.+\.cpp"
)

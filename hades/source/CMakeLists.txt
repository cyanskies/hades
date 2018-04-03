# Directory hades/source

# C++ source files
set(HADES_SRC
	Animation.cpp
	App.cpp
	archive.cpp
	Data.cpp
	Debug.cpp
	Collision.cpp
	Console.cpp
	ConsoleView.cpp
	common-input.cpp
	Curves.cpp
	data_system.cpp
  	ExportedCurves.cpp
	files.cpp
	GameInstance.cpp
	GameInterface.cpp
	GameRenderer.cpp
	GridArea.cpp
	Input.cpp
	Logging.cpp
	main.cpp
	parallel_jobs.cpp
	Properties.cpp
	simple_resources.cpp
	SpriteBatch.cpp
	StandardPaths.cpp
	State.cpp
	StateManager.cpp
	System.cpp
	TimerManager.cpp
	Types.cpp
	Objects/editor.cpp
	Objects/Objects.cpp
	Objects/resources.cpp
	OrthoTerrain/editor.cpp
	OrthoTerrain/terrain.cpp
	OrthoTerrain/utility.cpp
	OrthoTerrain/generator.cpp
	OrthoTerrain/resources.cpp
	Tiles/editor.cpp
	Tiles/generator.cpp
	Tiles/resources.cpp
	Tiles/serialise.cpp
	Tiles/tiles.cpp
)

source_group("Tiles" FILES
	Tiles/editor.cpp
	Tiles/generator.cpp
	Tiles/resources.cpp
	Tiles/serialise.cpp
	Tiles/tiles.cpp)

source_group("Objects" FILES
	Objects/editor.cpp
	Objects/Objects.cpp
	Objects/resources.cpp)

source_group("OrthoTerrain" FILES
	OrthoTerrain/editor.cpp
	OrthoTerrain/terrain.cpp
	OrthoTerrain/utility.cpp
	OrthoTerrain/generator.cpp
	OrthoTerrain/resources.cpp)

# Library to build
set(HADES_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${HADES_LIB} STATIC ${HADES_SRC})
set_target_properties(${HADES_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${HADES_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${HADES_LIB} PUBLIC ../include)

set(HADES_DEPS
	sfml-audio
	sfml-graphics
	sfml-main
	sfml-network
	sfml-system
	sfml-window
	sfgui
	yaml-cpp
	minizip)

#lin libraries
target_link_libraries(${HADES_LIB} PUBLIC ${HADES_DEPS})

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${HADES_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)

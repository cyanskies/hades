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
	DataManager.cpp
	data_manager.cpp
  	ExportedCurves.cpp
	files.cpp
	GameInstance.cpp
	GameInterface.cpp
	GameRenderer.cpp
	GuiLoaders.cpp
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
	Gui/Gui.cpp
	Gui/SimpleVerticalLayout.cpp
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

source_group("Gui" FILES
	Gui/Gui.cpp
	Gui/SimpleVerticalLayout.cpp)

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

if(WIN32)
	set(SFML_MAIN_RELEASE sfml-main)
	set(SFML_MAIN_DEBUG sfml-main-d)
	set(GLEW)
else()
	set(SFML_MAIN_RELEASE)
	set(SFML_MAIN_DEBUG)
	set(GLEW GLEW)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
	target_link_libraries(${HADES_LIB}
	${GLEW}
	sfml-system
	sfml-window
	sfml-graphics
	sfml-audio
	sfml-network
	${SFML_MAIN_RELEASE}
	tgui
	libyaml-cppmd
	zlibwapi
	)
else()
	target_link_libraries(${HADES_LIB}
	${GLEW}
	sfml-system-d
	sfml-window-d
	sfml-graphics-d
	sfml-audio-d
	sfml-network-d
	${SFML_MAIN_DEBUG}
	tgui-d
	libyaml-cppmdd
	zlibwapi
	)
endif()

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${HADES_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)

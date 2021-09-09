include(../cmake.txt)

set(HADES_SRC
	include/hades/App.hpp
	include/hades/Console.hpp
	include/hades/ConsoleView.hpp
	include/hades/data_system.hpp
	include/hades/Debug.hpp
	include/hades/Events.hpp
	include/hades/fps_display.hpp
	include/hades/Hades.hpp
	include/hades/LimitedDraw.hpp
	include/hades/Main.hpp
	include/hades/RenderInstance.hpp
	include/hades/Server.hpp
	include/hades/simple_resources.hpp
	include/hades/StateManager.hpp
	include/hades/yaml_parser.hpp
	include/hades/yaml_writer.hpp
	include/hades/detail/Console.inl
	include/hades/resource/fonts.hpp
	source/App.cpp
	source/Debug.cpp
	source/Console.cpp
	source/ConsoleView.cpp
	source/data_system.cpp
	source/fps_display.cpp
	source/main.cpp
	source/RenderInstance.cpp
	#source/ScriptManager.cpp
	source/Server.cpp
	source/simple_resources.cpp
	source/StateManager.cpp
	source/yaml_parser.cpp
	source/yaml_writer.cpp
)

set(HADES_LIBS 
	hades-util
	hades-basic
	hades-core
	PRIVATE yaml-cpp
)

hades_make_library(hades include "${HADES_SRC}" "${HADES_LIBS}")

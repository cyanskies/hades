include(../cmake.txt)

set(HADES_SRC
	include/hades/App.hpp
	include/hades/Console.hpp
	include/hades/data_system.hpp
	include/hades/fps_display.hpp
	include/hades/Main.hpp
	include/hades/Server.hpp
	include/hades/simple_resources.hpp
	include/hades/StateManager.hpp
	include/hades/yaml_parser.hpp
	include/hades/yaml_writer.hpp
	include/hades/detail/Console.inl
	include/hades/detail/data_system.inl
	include/hades/resource/fonts.hpp
	source/App.cpp
	source/Console.cpp
	source/data_system.cpp
	source/fps_display.cpp
	source/main.cpp
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
	hades-app
	PRIVATE yaml-cpp
)

hades_make_library(hades include "${HADES_SRC}" "${HADES_LIBS}")

include(../cmake.txt)

set(HADES_APP_SRC
	source/capabilities.cpp
	source/debug/console_overlay.cpp
	include/hades/capabilities.hpp
	include/hades/debug/console_overlay.hpp
)

set(HADES_APP_LIBS
	hades-core
	hades-gui
)

hades_make_library(hades-app include "${HADES_APP_SRC}" "${HADES_APP_LIBS}")

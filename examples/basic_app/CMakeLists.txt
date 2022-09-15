include(../../cmake.txt)

set(BASIC_APP_LIBS
	hades
	hades-app
)

hades_make_exe(basic-app "." "main.cpp" "${BASIC_APP_LIBS}")
set_target_properties(basic-app PROPERTIES FOLDER "hades-examples")


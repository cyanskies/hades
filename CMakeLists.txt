add_subdirectory(deps)
add_subdirectory(libs)
add_subdirectory(app)
add_subdirectory(hades)

#if defined build example
add_subdirectory(test)
add_subdirectory(examples)

#make visual studio debug the desired project by default
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT hades_example)
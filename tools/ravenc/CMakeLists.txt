# ravenc: the XMLC compiler (an .xmlc file -> one C file). See ravenc.c and cmake/RavenXMLC.cmake.
# It runs on the build machine and depends on nothing: it is installed in bin/ next to the library.
include(GNUInstallDirs)
add_executable(ravenc ravenc.c)
# MACOSX_BUNDLE OFF: a command-line tool, also when the project is built for iOS (where CMake makes every executable a bundle)
set_target_properties(ravenc PROPERTIES C_STANDARD 99 FOLDER "tools" MACOSX_BUNDLE OFF)
if(MSVC)
	target_compile_definitions(ravenc PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
install(TARGETS ravenc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

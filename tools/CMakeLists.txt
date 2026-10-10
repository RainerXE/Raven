# Raven tools.
#
#   raven_designer   edits the XML of an interface visually (tools/designer)
#
# embed.py and subset_font.py are the scripts that produce the files of
# src/embedded; they are not needed to build.

add_executable(raven_designer designer/main.c)
target_link_libraries(raven_designer PRIVATE raven::raven)
set_target_properties(raven_designer PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tools")
if(MSVC)
	target_compile_definitions(raven_designer PRIVATE _CRT_SECURE_NO_WARNINGS)
	# no console window behind the designer
	set_target_properties(raven_designer PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
elseif(MINGW)
	target_link_options(raven_designer PRIVATE -mwindows)
else()
	target_link_libraries(raven_designer PRIVATE m)
endif()

include(GNUInstallDirs)
install(TARGETS raven_designer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# The designer driving itself, without a window system and without a GPU:
# adds, moves, wraps, deletes, undoes, and checks the XML after each step.
if(RAVEN_BUILD_TESTS)
	add_test(NAME designer_script COMMAND raven_designer --null --frames 3 --script
		"f 2 path 0/0 set Text Bonjour expect Bonjour pick 2 do palette.add expect <Grid~Columns do edit.duplicate do edit.up do edit.wrap.Border expect <Border> do edit.unwrap do edit.delete do edit.undo do edit.undo do edit.redo do file.new.form expect <Field~Label path 2/1 set Hint typed expect Hint=\"typed\" do edit.duplicate do edit.out do edit.in do view.theme.glass_light do view.interact do view.run f 3 do view.theme.dark do file.new.panel expect <Border~Padding f 2")
endif()

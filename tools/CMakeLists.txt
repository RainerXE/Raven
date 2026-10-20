# Raven tools.
#
#   raven_designer   the IDE of Raven (tools/designer): CMake projects, interfaces
#                    edited visually (.xml, .xmlc), code, translations, build and run
#
# embed.py and subset_font.py are the scripts that produce the files of
# src/embedded; they are not needed to build.

add_executable(raven_designer designer/main.c designer/ide.h designer/os.c designer/project.c designer/editors.c
	designer/xmlc.c designer/build.c designer/strings.c designer/welcome.c)
# the headers of Raven, for the completion of its API (an installed IDE reads ../include)
target_compile_definitions(raven_designer PRIVATE "RAVEN_IDE_INCLUDE_DIR=\"${PROJECT_SOURCE_DIR}/include\"")
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
		"do file.new.window f 2 path 0/0 set Text Bonjour expect Bonjour pick 2 do palette.add expect <Grid~Columns do edit.duplicate do edit.up do edit.wrap.Border expect <Border> do edit.unwrap do edit.delete do edit.undo do edit.undo do edit.redo do file.new.form expect <Field~Label path 2/1 set Hint typed expect Hint=\"typed\" do edit.duplicate do edit.out do edit.in do view.theme.glass_light do view.interact do view.run f 3 do view.theme.dark do file.new.panel expect <Border~Padding f 2")

	# The IDE: a new project (CMakeLists.txt, main.xmlc, strings.csv), its interface and its
	# scripts, a translation edited and shown in the preview in another language, everything saved.
	set(_ide "${CMAKE_CURRENT_BINARY_DIR}/ide_test")
	add_test(NAME designer_ide_clean COMMAND "${CMAKE_COMMAND}" -E remove_directory "${_ide}")
	add_test(NAME designer_ide_folder COMMAND "${CMAKE_COMMAND}" -E make_directory "${_ide}")
	set_tests_properties(designer_ide_clean PROPERTIES FIXTURES_SETUP designer_ide_clean)
	set_tests_properties(designer_ide_folder PROPERTIES FIXTURES_SETUP designer_ide FIXTURES_REQUIRED designer_ide_clean)
	add_test(NAME designer_ide COMMAND raven_designer --null --frames 2 --script
		"f 2 project ${_ide} demo C expect @hello.button expectfile ${_ide}/demo/CMakeLists.txt raven_target_xmlc(demo) expectlabel status You~have~not~clicked~yet. cell hello.none 2 Rien~encore language fr f 1 expectlabel status Rien~encore language - f 1 expectlabel status You~have~not~clicked~yet. path 0/0 set Text Hi do file.save_all expectfile ${_ide}/demo/main.xmlc Text=\"Hi\" expectfile ${_ide}/demo/main.xmlc Rtr_plural(\"clicks\",~g_clicks) expectfile ${_ide}/demo/strings.csv ,Rien~encore f 2")
	set_tests_properties(designer_ide PROPERTIES FIXTURES_REQUIRED designer_ide)
endif()

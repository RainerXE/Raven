# Raven - XMLC: interfaces written in XML with their C code, compiled by ravenc.
#
#   add_executable(app main.xmlc other.c)
#   target_link_libraries(app PRIVATE raven::raven)
#   raven_target_xmlc(app)                 # every .xmlc (C) and .xmlc++/.xmlcpp (C++) among the sources of app
#   raven_target_xmlc(app more/panel.xmlc) # or the files given
#
# Each .xmlc becomes <build>/ravenc/<path>.xmlc.c (rebuilt when the .xmlc
# changes) and is compiled with the target; the compiler errors point into
# the .xmlc. A file "panel.xmlc" gives the function Rid panel_create(void).
# Plain .xml files stay interpreted at run time (Rwindow_create_from_xml_file).
#
# RAVEN_RAVENC: the ravenc program (set by find_package(raven), or the ravenc
# target when Raven is built in the same tree).

function(raven_target_xmlc target)
	if(TARGET ravenc)
		set(_ravenc "$<TARGET_FILE:ravenc>")
		set(_ravenc_dep ravenc)
	elseif(RAVEN_RAVENC)
		set(_ravenc "${RAVEN_RAVENC}")
		set(_ravenc_dep "${RAVEN_RAVENC}")
	else()
		message(FATAL_ERROR "raven_target_xmlc: ravenc was not found (RAVEN_RAVENC)")
	endif()

	set(_files ${ARGN})
	if(NOT _files)
		get_target_property(_sources ${target} SOURCES)
		foreach(_source IN LISTS _sources)
			if(_source MATCHES "\\.(xmlc|xmlc\\+\\+|xmlcpp)$")
				list(APPEND _files "${_source}")
			endif()
		endforeach()
	endif()

	foreach(_file IN LISTS _files)
		get_filename_component(_absolute "${_file}" ABSOLUTE)
		file(RELATIVE_PATH _relative "${CMAKE_CURRENT_SOURCE_DIR}" "${_absolute}")
		if(_relative MATCHES "^\\.\\." OR IS_ABSOLUTE "${_relative}")
			# outside of this folder: by its name
			get_filename_component(_relative "${_absolute}" NAME)
		endif()
		# C++ for .xmlc++ and .xmlcpp, C for .xmlc
		if(_absolute MATCHES "\\.(xmlc\\+\\+|xmlcpp)$")
			string(REPLACE "+" "p" _relative "${_relative}")
			set(_output "${CMAKE_CURRENT_BINARY_DIR}/ravenc/${_relative}.cpp")
		else()
			set(_output "${CMAKE_CURRENT_BINARY_DIR}/ravenc/${_relative}.c")
		endif()
		get_filename_component(_output_dir "${_output}" DIRECTORY)
		add_custom_command(
			OUTPUT "${_output}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${_output_dir}"
			COMMAND ${_ravenc} "${_absolute}" -o "${_output}"
			DEPENDS "${_absolute}" ${_ravenc_dep}
			COMMENT "ravenc ${_relative}"
			VERBATIM)
		# the .xmlc stays in the project (IDE), the generated C is what is compiled
		target_sources(${target} PRIVATE "${_absolute}" "${_output}")
		set_source_files_properties("${_absolute}" PROPERTIES HEADER_FILE_ONLY ON)
		set_source_files_properties("${_output}" PROPERTIES GENERATED ON)
		source_group("XMLC" FILES "${_absolute}")
		source_group("XMLC\\generated" FILES "${_output}")
	endforeach()
endfunction()

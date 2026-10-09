# Recompiles the GLSL shaders of the Vulkan backend into the SPIR-V headers
# kept in the repository (src/render/vulkan/generated). Only used when
# HUI_REBUILD_SHADERS is ON: a normal build needs no shader compiler.

find_program(HUI_GLSLANG NAMES glslangValidator glslang HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin bin)
find_program(HUI_GLSLC NAMES glslc HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin bin)
if(NOT HUI_GLSLANG AND NOT HUI_GLSLC)
	message(FATAL_ERROR "HUI_REBUILD_SHADERS needs glslangValidator or glslc (both come with the Vulkan SDK)")
endif()

# hui_compile_shader(<source.glsl> <output header> <C symbol>)
function(hui_compile_shader source header symbol)
	set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
	set(header_path "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
	get_filename_component(name "${source}" NAME)
	set(spirv "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv")
	if(HUI_GLSLANG)
		set(compile ${HUI_GLSLANG} -V --quiet -o "${spirv}" "${source_path}")
	else()
		set(compile ${HUI_GLSLC} -O -o "${spirv}" "${source_path}")
	endif()
	add_custom_command(
		OUTPUT "${header_path}"
		COMMAND ${compile}
		COMMAND ${CMAKE_COMMAND} -DINPUT=${spirv} -DOUTPUT=${header_path} -DSYMBOL=${symbol} -DSOURCE=${source}
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HuiEmbedSpirv.cmake"
		DEPENDS "${source_path}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HuiEmbedSpirv.cmake"
		COMMENT "HUI: compiling ${source}"
		VERBATIM
	)
	target_sources(hui PRIVATE "${header_path}")
endfunction()

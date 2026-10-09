# HUI examples.
#
#   hello            a window built with the C API
#   editor           a window described in XML, software-rendered viewport,
#                    custom widget, virtual list
#   viewport_vulkan  a Vulkan scene shown in the UI (offscreen image + in-pass draw)
#   viewport_opengl  an OpenGL scene shown in a native child window

function(hui_add_example name)
	add_executable(hui_${name} ${ARGN} common/example.h)
	target_link_libraries(hui_${name} PRIVATE hui::hui)
	set_target_properties(hui_${name} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
	# where the example finds its data files when started from any folder
	target_compile_definitions(hui_${name} PRIVATE HUI_EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/${name}")
	if(MSVC)
		target_compile_definitions(hui_${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
	else()
		target_link_libraries(hui_${name} PRIVATE m)
	endif()
endfunction()

hui_add_example(hello hello/main.c)
hui_add_example(editor editor/main.c editor/editor.xml)

# The Vulkan example calls Vulkan itself, so it needs the Vulkan library
# (the Vulkan SDK on Windows, libvulkan-dev on Linux). HUI does not.
if(HUI_BACKEND_VULKAN)
	find_package(Vulkan QUIET)
	if(Vulkan_FOUND)
		hui_add_example(viewport_vulkan viewport_vulkan/main.c viewport_vulkan/shaders_spv.h)
		target_link_libraries(hui_viewport_vulkan PRIVATE Vulkan::Vulkan)
		if(NOT WIN32)
			find_package(Threads REQUIRED)
			target_link_libraries(hui_viewport_vulkan PRIVATE Threads::Threads)
		endif()
	else()
		message(STATUS "HUI: example viewport_vulkan skipped (Vulkan library not found)")
	endif()
endif()

# The OpenGL example creates its context on the native child window of a viewport.
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL QUIET)
if(OpenGL_FOUND AND (WIN32 OR HUI_PLATFORM_SELECTED STREQUAL "x11"))
	hui_add_example(viewport_opengl viewport_opengl/main.c)
	target_link_libraries(hui_viewport_opengl PRIVATE OpenGL::GL)
	if(WIN32)
		target_link_libraries(hui_viewport_opengl PRIVATE gdi32 user32)
	else()
		target_include_directories(hui_viewport_opengl PRIVATE ${X11_INCLUDE_DIR})
		target_link_libraries(hui_viewport_opengl PRIVATE ${X11_X11_LIB})
	endif()
else()
	message(STATUS "HUI: example viewport_opengl skipped (OpenGL not found)")
endif()

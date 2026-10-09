cmake_minimum_required(VERSION 3.16)

project(hui VERSION 0.1.0 DESCRIPTION "Horizon UI - lightweight multithreaded GUI library in C" LANGUAGES C)

# ---------------------------------------------------------------------------
#  Options
# ---------------------------------------------------------------------------

# ON when HUI is the project being built, OFF when it is added with add_subdirectory
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(HUI_IS_TOP_LEVEL ON)
else()
	set(HUI_IS_TOP_LEVEL OFF)
endif()

option(HUI_BUILD_SHARED "Build HUI as a shared library (hui.dll / libhui.so)" OFF)
option(HUI_BUILD_EXAMPLES "Build the example programs" ${HUI_IS_TOP_LEVEL})
option(HUI_BUILD_TESTS "Build the tests and the benchmark" ${HUI_IS_TOP_LEVEL})
option(HUI_BACKEND_VULKAN "Compile the Vulkan backend" ON)
option(HUI_WITH_STB_IMAGE "Decode PNG / JPEG / BMP / TGA textures with stb_image" ON)
option(HUI_FETCH_VULKAN_HEADERS "Download the Vulkan headers when no Vulkan SDK is found" ON)
option(HUI_REBUILD_SHADERS "Recompile the GLSL shaders (needs glslangValidator or glslc)" OFF)
option(HUI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
set(HUI_PLATFORM "auto" CACHE STRING "Window system: auto, win32, x11 or null")
set_property(CACHE HUI_PLATFORM PROPERTY STRINGS auto win32 x11 null)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(HuiCompilerOptions)

# ---------------------------------------------------------------------------
#  Window system
# ---------------------------------------------------------------------------

if(HUI_PLATFORM STREQUAL "auto")
	if(WIN32)
		set(HUI_PLATFORM_SELECTED "win32")
	elseif(UNIX AND NOT APPLE)
		set(HUI_PLATFORM_SELECTED "x11")
	else()
		set(HUI_PLATFORM_SELECTED "null")
		message(WARNING "HUI: no window system is implemented for this OS yet: building the headless (null) platform only.")
	endif()
else()
	set(HUI_PLATFORM_SELECTED "${HUI_PLATFORM}")
endif()

# ---------------------------------------------------------------------------
#  Sources
# ---------------------------------------------------------------------------

set(HUI_PUBLIC_HEADERS
	include/hui/hui.h
	include/hui/hui_core.h
	include/hui/hui_window.h
	include/hui/hui_style.h
	include/hui/hui_resources.h
	include/hui/hui_widget.h
	include/hui/hui_layout.h
	include/hui/hui_controls.h
	include/hui/hui_viewport.h
	include/hui/hui_draw.h
	include/hui/hui_xml.h
	include/hui/hui_backend.h
	include/hui/hui_vulkan.h
)

set(HUI_CORE_SOURCES
	src/core/base.h
	src/core/context.c
	src/core/context.h
	src/core/error.c
	src/core/file.c
	src/core/handle.c
	src/core/handle.h
	src/core/jobs.c
	src/core/jobs.h
	src/core/lfmap.c
	src/core/lfmap.h
	src/core/memory.c
	src/core/thread.h
	src/core/utf8.c
)

set(HUI_PLATFORM_SOURCES
	src/platform/platform.c
	src/platform/platform.h
	src/platform/null/null_platform.c
)

set(HUI_RENDER_SOURCES
	src/render/draw.c
	src/render/draw.h
	src/render/font.c
	src/render/font.h
	src/render/font_raster.c
	src/render/image_decode.c
	src/render/renderer.c
	src/render/renderer.h
	src/render/texture.c
	src/render/null/null_backend.c
	src/resources/default_font.c
)

set(HUI_WIDGET_SOURCES
	src/widget/events.c
	src/widget/layout.c
	src/widget/paint.c
	src/widget/registry.c
	src/widget/style.c
	src/widget/widget.c
	src/widget/widget.h
	src/widget/window.c
)

set(HUI_WIDGETS_SOURCES
	src/widgets/common.c
	src/widgets/common.h
	src/widgets/textedit.c
	# panels
	src/widgets/border.c
	src/widgets/box_panel.c
	src/widgets/canvas.c
	src/widgets/expander.c
	src/widgets/grid.c
	src/widgets/overlay.c
	src/widgets/scroll_box.c
	src/widgets/size_box.c
	src/widgets/splitter.c
	src/widgets/switcher.c
	src/widgets/tab_view.c
	src/widgets/wrap_box.c
	# controls
	src/widgets/button.c
	src/widgets/checkbox.c
	src/widgets/combo_box.c
	src/widgets/image.c
	src/widgets/label.c
	src/widgets/list_view.c
	src/widgets/menu.c
	src/widgets/progress_bar.c
	src/widgets/slider.c
	src/widgets/spin_box.c
	src/widgets/text_input.c
	src/widgets/tree_view.c
	src/widgets/viewport.c
)

set(HUI_XML_SOURCES
	src/xml/xml.h
	src/xml/xml_attributes.c
	src/xml/xml_loader.c
	src/xml/xml_parser.c
)

set(HUI_DEFINITIONS "")
set(HUI_LIBRARIES "")

# threads
if(WIN32)
	list(APPEND HUI_CORE_SOURCES src/core/thread_win32.c)
else()
	list(APPEND HUI_CORE_SOURCES src/core/thread_posix.c)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	list(APPEND HUI_LIBRARIES Threads::Threads)
	# libm, and libdl for loading the Vulkan library at run time
	list(APPEND HUI_LIBRARIES m ${CMAKE_DL_LIBS})
endif()

# window system
if(HUI_PLATFORM_SELECTED STREQUAL "win32")
	list(APPEND HUI_PLATFORM_SOURCES
		src/platform/win32/win32_platform.c
	)
	list(APPEND HUI_DEFINITIONS HUI_PLATFORM_WIN32)
	list(APPEND HUI_LIBRARIES user32 gdi32)
elseif(HUI_PLATFORM_SELECTED STREQUAL "x11")
	find_package(X11 REQUIRED)
	list(APPEND HUI_PLATFORM_SOURCES
		src/platform/x11/x11_platform.c
		src/platform/x11/x11_keys.c
		src/platform/x11/x11_internal.h
	)
	list(APPEND HUI_DEFINITIONS HUI_PLATFORM_X11)
	list(APPEND HUI_LIBRARIES ${X11_X11_LIB})
elseif(NOT HUI_PLATFORM_SELECTED STREQUAL "null")
	message(FATAL_ERROR "HUI: unknown HUI_PLATFORM '${HUI_PLATFORM_SELECTED}' (expected auto, win32, x11 or null)")
endif()

# Vulkan backend: only the headers are needed, the library is loaded at run time
if(HUI_BACKEND_VULKAN)
	include(HuiVulkanHeaders)
	list(APPEND HUI_RENDER_SOURCES
		src/render/vulkan/vk_backend.c
		src/render/vulkan/vk_backend.h
		src/render/vulkan/vk_device.c
		src/render/vulkan/vk_frame.c
		src/render/vulkan/vk_loader.c
		src/render/vulkan/vk_loader.h
		src/render/vulkan/vk_pipeline.c
		src/render/vulkan/vk_public.c
		src/render/vulkan/vk_surface.c
		src/render/vulkan/vk_texture.c
	)
	list(APPEND HUI_DEFINITIONS HUI_BACKEND_VULKAN_ENABLED)
	if(HUI_REBUILD_SHADERS)
		include(HuiShaders)
		hui_compile_shader(src/render/vulkan/shaders/ui.vert src/render/vulkan/generated/ui_vert_spv.h hui_ui_vert_spv)
		hui_compile_shader(src/render/vulkan/shaders/ui.frag src/render/vulkan/generated/ui_frag_spv.h hui_ui_frag_spv)
	endif()
endif()

if(HUI_WITH_STB_IMAGE)
	list(APPEND HUI_DEFINITIONS HUI_WITH_STB_IMAGE)
endif()

# ---------------------------------------------------------------------------
#  Library
# ---------------------------------------------------------------------------

if(HUI_BUILD_SHARED)
	add_library(hui SHARED)
	target_compile_definitions(hui PRIVATE HUI_BUILD_DLL INTERFACE HUI_USE_DLL)
	set_target_properties(hui PROPERTIES C_VISIBILITY_PRESET hidden)
else()
	add_library(hui STATIC)
endif()
add_library(hui::hui ALIAS hui)

target_sources(hui PRIVATE
	${HUI_PUBLIC_HEADERS}
	${HUI_CORE_SOURCES}
	${HUI_PLATFORM_SOURCES}
	${HUI_RENDER_SOURCES}
	${HUI_WIDGET_SOURCES}
	${HUI_WIDGETS_SOURCES}
	${HUI_XML_SOURCES}
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
	${HUI_PUBLIC_HEADERS} ${HUI_CORE_SOURCES} ${HUI_PLATFORM_SOURCES} ${HUI_RENDER_SOURCES}
	${HUI_WIDGET_SOURCES} ${HUI_WIDGETS_SOURCES} ${HUI_XML_SOURCES})

target_include_directories(hui
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
target_compile_definitions(hui PRIVATE ${HUI_DEFINITIONS})
target_link_libraries(hui PRIVATE ${HUI_LIBRARIES})
set_target_properties(hui PROPERTIES
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF
	POSITION_INDEPENDENT_CODE ON
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)
hui_set_compiler_options(hui)

if(HUI_PLATFORM_SELECTED STREQUAL "x11")
	target_include_directories(hui PRIVATE ${X11_INCLUDE_DIR})
endif()
if(HUI_BACKEND_VULKAN)
	target_include_directories(hui PRIVATE ${HUI_VULKAN_INCLUDE_DIR})
endif()

message(STATUS "HUI ${PROJECT_VERSION}: platform=${HUI_PLATFORM_SELECTED} vulkan=${HUI_BACKEND_VULKAN} shared=${HUI_BUILD_SHARED}")

# ---------------------------------------------------------------------------
#  Examples and tests
# ---------------------------------------------------------------------------

if(HUI_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(HUI_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
#  Install:  find_package(hui)  +  target_link_libraries(app PRIVATE hui::hui)
# ---------------------------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS hui EXPORT huiTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/hui DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT huiTargets NAMESPACE hui:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hui)

configure_package_config_file(cmake/huiConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/huiConfig.cmake"
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hui)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/huiConfigVersion.cmake"
	VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/huiConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/huiConfigVersion.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hui)

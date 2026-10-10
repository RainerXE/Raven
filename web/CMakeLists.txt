# Raven Web: web pages inside a Raven interface (see include/raven_web.h).
#
# An optional library next to Raven: it is built with the option
# RAVEN_BUILD_WEB, and an application uses it by linking raven::web as well
# as raven::raven. It needs nothing at build time; at run time it drives a
# browser of the Chromium family that is installed on the computer.
#
#   target_link_libraries(app PRIVATE raven::raven raven::web)

set(RAVEN_WEB_SOURCES
	include/raven_web.h
	src/web_internal.h
	src/web_json.c
	src/web_browser.c
	src/web_view.c
	src/web_posix.c
	src/web_win32.c
)

if(RAVEN_BUILD_SHARED)
	add_library(raven_web SHARED ${RAVEN_WEB_SOURCES})
	target_compile_definitions(raven_web PRIVATE RAVEN_WEB_BUILD_DLL INTERFACE RAVEN_WEB_USE_DLL)
	set_target_properties(raven_web PROPERTIES C_VISIBILITY_PRESET hidden)
else()
	add_library(raven_web STATIC ${RAVEN_WEB_SOURCES})
endif()
add_library(raven::web ALIAS raven_web)
set_target_properties(raven_web PROPERTIES EXPORT_NAME web)

target_include_directories(raven_web PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
target_link_libraries(raven_web PUBLIC raven)
set_target_properties(raven_web PROPERTIES
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
	POSITION_INDEPENDENT_CODE ON
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	FOLDER "web"
)
raven_set_compiler_options(raven_web)
if(NOT RAVEN_WITH_STB_IMAGE)
	message(WARNING "raven_web: Raven is built without its image decoder (RAVEN_WITH_STB_IMAGE=OFF): web views will work but show nothing, the pictures of the pages cannot be decoded.")
endif()
if(NOT WIN32)
	find_package(Threads REQUIRED)
	target_link_libraries(raven_web PRIVATE Threads::Threads m)
endif()
if(MINGW AND RAVEN_BUILD_SHARED)
	set_target_properties(raven_web PROPERTIES PREFIX "")
	target_link_options(raven_web PRIVATE -static-libgcc)
endif()

install(TARGETS raven_web EXPORT ravenTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES include/raven_web.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# The test: the parts that need no browser always run; the rest is reported
# as skipped on a machine without a browser of the Chromium family.
if(RAVEN_BUILD_TESTS)
	add_executable(test_web tests/test_web.c src/web_json.c)
	target_link_libraries(test_web PRIVATE raven::web raven::raven)
	target_include_directories(test_web PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../tests")
	set_target_properties(test_web PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tests")
	if(NOT WIN32)
		target_link_libraries(test_web PRIVATE m)
	endif()
	if(MSVC)
		target_compile_definitions(test_web PRIVATE _CRT_SECURE_NO_WARNINGS)
	endif()
	if(NOT RAVEN_WITH_STB_IMAGE)
		target_compile_definitions(test_web PRIVATE RAVEN_TEST_NO_IMAGE_DECODER)
	endif()
	add_test(NAME test_web COMMAND test_web)
	set_tests_properties(test_web PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()

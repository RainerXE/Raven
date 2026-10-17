# Raven - the license tools.
#
#   raven_license_check   ships with Raven (bin/): find_package(raven) runs it
#                         to check a license and decrypt the Release edition.
#   raven_license         the author's tool: keys, licenses, encryption. Built,
#                         never installed or packaged.
#
# The public key comes from tools/license/public.key (made by
# "raven_license keygen"). Without it, raven_license_check is not built and
# the packages cannot offer the Release edition (dist.bat / dist.sh stop).

add_library(raven_tweetnacl STATIC "${PROJECT_SOURCE_DIR}/third_party/tweetnacl/tweetnacl.c")
target_include_directories(raven_tweetnacl PUBLIC "${PROJECT_SOURCE_DIR}/third_party/tweetnacl")
set_target_properties(raven_tweetnacl PROPERTIES C_STANDARD 11 FOLDER "tools" POSITION_INDEPENDENT_CODE ON)
if(MSVC)
	target_compile_options(raven_tweetnacl PRIVATE /w)
else()
	target_compile_options(raven_tweetnacl PRIVATE -w)
endif()

add_executable(raven_license raven_license.c license_format.h)
target_link_libraries(raven_license PRIVATE raven_tweetnacl)
set_target_properties(raven_license PROPERTIES C_STANDARD 11 FOLDER "tools")
if(WIN32)
	target_link_libraries(raven_license PRIVATE bcrypt)
endif()
if(MSVC)
	target_compile_definitions(raven_license PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set(RAVEN_LICENSE_PUBLIC_KEY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/public.key")
if(EXISTS "${RAVEN_LICENSE_PUBLIC_KEY_FILE}")
	file(READ "${RAVEN_LICENSE_PUBLIC_KEY_FILE}" RAVEN_LICENSE_PUBLIC_KEY)
	string(STRIP "${RAVEN_LICENSE_PUBLIC_KEY}" RAVEN_LICENSE_PUBLIC_KEY)
endif()
string(LENGTH "${RAVEN_LICENSE_PUBLIC_KEY}" RAVEN_LICENSE_PUBLIC_KEY_LENGTH)
if(RAVEN_LICENSE_PUBLIC_KEY MATCHES "^[0-9a-fA-F]+$" AND RAVEN_LICENSE_PUBLIC_KEY_LENGTH EQUAL 64)
	add_executable(raven_license_check raven_license_check.c license_format.h)
	target_link_libraries(raven_license_check PRIVATE raven_tweetnacl)
	target_compile_definitions(raven_license_check PRIVATE "RAVEN_LICENSE_PUBLIC_KEY=\"${RAVEN_LICENSE_PUBLIC_KEY}\"")
	set_target_properties(raven_license_check PROPERTIES C_STANDARD 11 FOLDER "tools")
	if(MSVC)
		target_compile_definitions(raven_license_check PRIVATE _CRT_SECURE_NO_WARNINGS)
		# (no C runtime DLL beside it in the package)
		set_property(TARGET raven_license_check PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
	elseif(MINGW)
		target_link_options(raven_license_check PRIVATE -static)
	endif()
	install(TARGETS raven_license_check RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	set(RAVEN_HAS_LICENSE_CHECK ON PARENT_SCOPE)
else()
	message(WARNING "Raven: tools/license/public.key is missing or invalid: raven_license_check is not built, "
		"and the packages cannot offer the Release edition. Run \"raven_license keygen\" once, then copy public.key there.")
	set(RAVEN_HAS_LICENSE_CHECK OFF PARENT_SCOPE)
endif()

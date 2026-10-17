# Raven - tools/license/encrypt_release.cmake
# Puts the Release edition into a package, encrypted: every library of the
# Release build (STAGE, an install of it) that differs from the same file of
# the package (OUT, the license-free install) is encrypted beside it as
# <file>.rvnenc. find_package(raven) decrypts it when given a license.
#
#   cmake -DTOOL=<raven_license> -DSTAGE=<release install> -DOUT=<package> [-DKEYS=<folder>]
#         -P tools/license/encrypt_release.cmake
#
# Used by dist.bat and dist.sh. Needs the unlock key (raven_license keygen).

foreach(var TOOL STAGE OUT)
	if(NOT ${var})
		message(FATAL_ERROR "encrypt_release.cmake: -D${var}=... is missing")
	endif()
endforeach()
set(keys_args "")
if(KEYS)
	set(keys_args --keys "${KEYS}")
endif()

file(GLOB_RECURSE candidates RELATIVE "${STAGE}"
	"${STAGE}/*.a" "${STAGE}/*.lib" "${STAGE}/*.dll" "${STAGE}/*.so" "${STAGE}/*.so.*" "${STAGE}/*.dylib")
set(count 0)
foreach(rel IN LISTS candidates)
	if(IS_SYMLINK "${STAGE}/${rel}" OR NOT EXISTS "${OUT}/${rel}")
		continue()
	endif()
	execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${STAGE}/${rel}" "${OUT}/${rel}" RESULT_VARIABLE same)
	if(same EQUAL 0)
		continue()   # the same in both editions (an import library...)
	endif()
	execute_process(COMMAND "${TOOL}" encrypt ${keys_args} "${STAGE}/${rel}" "${OUT}/${rel}.rvnenc" RESULT_VARIABLE result)
	if(NOT result EQUAL 0)
		message(FATAL_ERROR "encrypt_release.cmake: could not encrypt ${rel}")
	endif()
	math(EXPR count "${count} + 1")
endforeach()
if(count EQUAL 0)
	message(FATAL_ERROR "encrypt_release.cmake: no library differs between the two editions in ${STAGE}")
endif()
message(STATUS "Release edition: ${count} encrypted librar(y/ies) in ${OUT}")

# Locates the Vulkan headers and sets HUI_VULKAN_INCLUDE_DIR.
#
# HUI only needs the headers: the Vulkan library (vulkan-1.dll, libvulkan.so.1)
# is loaded at run time, so nothing is linked. Search order:
#   1. HUI_VULKAN_INCLUDE_DIR given on the command line
#   2. an installed Vulkan SDK / system package (find_package(Vulkan))
#   3. the Khronos Vulkan-Headers repository, downloaded with FetchContent

if(HUI_VULKAN_INCLUDE_DIR)
	message(STATUS "HUI: Vulkan headers from HUI_VULKAN_INCLUDE_DIR = ${HUI_VULKAN_INCLUDE_DIR}")
else()
	find_package(Vulkan QUIET)
	if(Vulkan_INCLUDE_DIRS)
		set(HUI_VULKAN_INCLUDE_DIR "${Vulkan_INCLUDE_DIRS}")
		message(STATUS "HUI: Vulkan headers found in ${HUI_VULKAN_INCLUDE_DIR}")
	elseif(HUI_FETCH_VULKAN_HEADERS)
		include(FetchContent)
		message(STATUS "HUI: no Vulkan SDK found, fetching Khronos Vulkan-Headers")
		# Only the include folder is needed: the CMake project of the repository must not be configured.
		if(CMAKE_VERSION VERSION_LESS 3.18)
			FetchContent_Declare(hui_vulkan_headers
				GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
				GIT_TAG v1.3.290
				GIT_SHALLOW TRUE
			)
			FetchContent_GetProperties(hui_vulkan_headers)
			if(NOT hui_vulkan_headers_POPULATED)
				FetchContent_Populate(hui_vulkan_headers)
			endif()
		else()
			# a SOURCE_SUBDIR without CMakeLists.txt makes FetchContent_MakeAvailable download only
			FetchContent_Declare(hui_vulkan_headers
				GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
				GIT_TAG v1.3.290
				GIT_SHALLOW TRUE
				SOURCE_SUBDIR hui-headers-only
			)
			FetchContent_MakeAvailable(hui_vulkan_headers)
		endif()
		set(HUI_VULKAN_INCLUDE_DIR "${hui_vulkan_headers_SOURCE_DIR}/include")
	else()
		message(FATAL_ERROR
			"HUI: the Vulkan headers were not found. Install the Vulkan SDK, pass "
			"-DHUI_VULKAN_INCLUDE_DIR=<folder containing vulkan/vulkan.h>, enable "
			"HUI_FETCH_VULKAN_HEADERS, or disable the backend with -DHUI_BACKEND_VULKAN=OFF.")
	endif()
endif()

if(NOT EXISTS "${HUI_VULKAN_INCLUDE_DIR}/vulkan/vulkan.h")
	message(FATAL_ERROR "HUI: ${HUI_VULKAN_INCLUDE_DIR} does not contain vulkan/vulkan.h")
endif()

cmake_minimum_required(VERSION 3.16)
project(hello_cpp CXX)

# Raven: the package (bin/, include/, lib/). The IDE gives its folder in CMAKE_PREFIX_PATH.
find_package(raven REQUIRED)

# main.xmlc++ holds the interface and its code: ravenc turns it into C++ (raven_target_xmlc)
add_executable(hello_cpp main.xmlc++ strings.csv)
target_link_libraries(hello_cpp PRIVATE raven::raven)
raven_target_xmlc(hello_cpp)
if(MSVC)
	target_compile_definitions(hello_cpp PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

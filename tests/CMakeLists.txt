# Tests.
#
#   test_layout, test_widgets, test_xml, test_threads
#       run headless (null platform + null backend): no display and no GPU are
#       needed, so they run on any build machine.
#   test_render
#       renders real frames with the Vulkan backend and checks pixels. It is
#       reported as skipped when the machine has no usable Vulkan driver.
#
#   raven_benchmark
#       not a test: prints layout and paint timings for a large widget tree.
#
#   ctest --output-on-failure

function(raven_add_test name)
	add_executable(${name} ${name}.c test.h)
	target_link_libraries(${name} PRIVATE raven::raven)
	set_target_properties(${name} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tests")
	if(NOT WIN32)
		target_link_libraries(${name} PRIVATE m)
	endif()
	if(MSVC)
		target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
	endif()
	if(NOT RAVEN_WITH_STB_IMAGE)
		target_compile_definitions(${name} PRIVATE RAVEN_TEST_NO_IMAGE_DECODER)
	endif()
	add_test(NAME ${name} COMMAND ${name})
endfunction()

raven_add_test(test_layout)
raven_add_test(test_widgets)
raven_add_test(test_xml)
raven_add_test(test_threads)

if(RAVEN_BACKEND_VULKAN)
	raven_add_test(test_render)
	set_tests_properties(test_render PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Not a test: prints timings for a large widget tree (see the top of benchmark.c).
add_executable(raven_benchmark benchmark.c)
target_link_libraries(raven_benchmark PRIVATE raven::raven)
set_target_properties(raven_benchmark PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tests")
if(MSVC)
	target_compile_definitions(raven_benchmark PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

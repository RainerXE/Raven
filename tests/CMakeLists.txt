# Tests.
#
#   test_layout, test_widgets, test_xml, test_threads
#       run headless (null platform + null backend): no display and no GPU are
#       needed, so they run on any build machine.
#   test_render
#       renders real frames with the Vulkan backend and checks pixels. It is
#       reported as skipped when the machine has no usable Vulkan driver.
#
#   hui_benchmark
#       not a test: prints layout and paint timings for a large widget tree.
#
#   ctest --output-on-failure

function(hui_add_test name)
	add_executable(${name} ${name}.c test.h)
	target_link_libraries(${name} PRIVATE hui::hui)
	set_target_properties(${name} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
	if(NOT WIN32)
		target_link_libraries(${name} PRIVATE m)
	endif()
	if(MSVC)
		target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
	endif()
	if(NOT HUI_WITH_STB_IMAGE)
		target_compile_definitions(${name} PRIVATE HUI_TEST_NO_IMAGE_DECODER)
	endif()
	add_test(NAME ${name} COMMAND ${name})
endfunction()

hui_add_test(test_layout)
hui_add_test(test_widgets)
hui_add_test(test_xml)
hui_add_test(test_threads)

if(HUI_BACKEND_VULKAN)
	hui_add_test(test_render)
	set_tests_properties(test_render PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Not a test: prints timings for a large widget tree (see the top of benchmark.c).
add_executable(hui_benchmark benchmark.c)
target_link_libraries(hui_benchmark PRIVATE hui::hui)
set_target_properties(hui_benchmark PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
	target_compile_definitions(hui_benchmark PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

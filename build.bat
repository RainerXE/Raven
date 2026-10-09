cd build
cmake -S .. -B . -DHUI_BUILD_EXAMPLES=ON -DHUI_BUILD_TESTS=ON
cmake --build . -j12
pause
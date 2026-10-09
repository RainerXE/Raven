cd build
cmake -S .. -B . -DRAVEN_BUILD_EXAMPLES=ON
cmake --build . -j12
pause
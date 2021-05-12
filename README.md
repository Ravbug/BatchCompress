# BatchCompress
Inspired by [ImageOptim](https://imageoptim.com/mac), but cross-platform written in C++17. Uses wxWidgets for native GUI on all platforms.

Supported formats:
- PNG

## Compiling
Use CMake:
```sh
mkdir -p build
cd build
cmake .. 
cmake --build . --config Release --target install
```
The install target does not write any files outside of the build directory. 

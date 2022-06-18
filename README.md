## Realtime raytracing

### Control camera view:

Rotate camera with mouse

Movements:

- WASD
- Space - up
- Ctrl - down
- Shift (hold) - object's speed boost
- Alt (hold) - object's speed slowdown

### Requirements

* CMake (>= 3.0.2). Download and install for Windows x64: [here](https://github.com/Kitware/CMake/releases/download/v3.24.0-rc1/cmake-3.24.0-rc1-windows-x86_64.msi). Remember add PATH.
* GPU with OpenGL (>= 3.3) support
* GLM (included)
* GLFW, should be automatically found by CMake (win64 binaries included)

### Build
First, run cmd on folder /src/

Second, 
```sh
mkdir bin
cd bin
cmake ..
cmake --build .
```

Next, run file bin/RayTracing_OpenGL.sln
### Release
- Download quick review version at [here]()

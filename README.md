# McEngine (needs updating)
A small cross-platform engine/framework/sandbox for drawing stuff to a window with GPU acceleration, handling various input devices, outputting sound, and networking. Mostly aimed at game development, although it does not (and will never) have a visual editor like Unity, Unreal or CryEngine, so everything is written in C++. It's more or less my little coding playground, a place to experiment and improve over time, and may be useful to other people as well. As it contains code from since I started learning programming (2009) to today, quality varies a bit across not-yet-refactored-and-cleaned-up files.

## Building

### Linux
After installing dependencies, just run `./configure && make -j$(nproc) && make install`, which will put the build output in `dist` by default. The build system also supports standard out-of-tree builds, useful for development to avoid polluting the main source directory. Check out [autogen.sh](autogen.sh) for advice. You could also look at `./configure --help` for some configurable build-time options.

You'll need to get the assets from the Steam version of McOsu -- an easy way to do that is to just overwrite your Steam installation: `cp -r dist/bin/* <path-to-steam-mcosu>/`

Required system dependencies:

`libX11 libXi libGL libGLU libEGL libjpeg libz`

Current list of deps which can either come from the system, or be built from source (e.g. `./configure --disable-system-deps`, or if missing on host):

`sdl3 GLEW freetype2`

Ubuntu example:

`sudo apt-get install build-essential libx11-dev libxi-dev libglu1-mesa-dev`


**Design Philosophy:**

- Very thin wrappers around OS and graphics APIs (e.g. at most one level of indirection or vtable lookup)
- Lean dependencies (e.g. a C++ compiler/toolchain is the only requirement to get started, all dependencies are wrapped and replaceable)
- No preempting of any specific development or architectural game code style (e.g. there will never be a generic "Entity" class of any kind)
- Rapid prototyping (e.g. loading and drawing an image is two lines of code, moving the image with a key three more)

**About the structure:**

Inside the McEngine project folder are two subfolders, ```libraries``` &amp; ```src```.

1. ```/libraries/``` contains the proprietary BASS dependencies

2. ```/src/``` contains the source code
   1. ```/src/App/``` contains the code for McOsu
   2. ```/src/Engine/``` contains the core
      1. ```/src/Engine/Input/``` contains input devices
      2. ```/src/Engine/Main/``` contains the main entry points
      3. ```/src/Engine/Platform/``` contains all platform specific code which is not in Main
      4. ```/src/Engine/Renderer/``` contains renderer specific code
   3. ```/src/GUI/``` contains very primitive UI elements, mostly for debugging (such as the console)
   4. ```/src/Util/``` contains helper functions, and small libraries which are header-only or contained in one file

- Every supported platform must have a ```main_<OS_NAME>.cpp``` file in ```/src/Engine/Main/``` containing the main entry point.
- Other platform specific code which is not part of the main file goes into ```/src/Engine/Platform/```.
- Hiding platform specific code is done by using trivial ```#ifdefs```, meaning that the exact same codebase can instantly be compiled without any changes on different platforms.
- I don't like relative includes, therefore _every single (sub)directory_ which is needed is added as an include path to the compiler.
- Separate applications using the engine go into ```/src/App/```. The ```FrameworkTest``` app is hardcoded by default.
- Every application must be started by including its header in ```/src/Engine/Engine.cpp``` as well as instantiating it in ```Engine::loadApp()``` atm.
- Not all libraries are necessary for every project, you can enable or disable certain libraries completely by using defines in ```/src/Engine/Main/EngineFeatures.h```, such as OpenCL, OpenVR, XInput, BASS or ENet. Everything will always compile and run, even if you use features which are not enabled.

**Projects using the engine:**

[https://github.com/McKay42/McOsu](https://github.com/McKay42/McOsu)

[https://github.com/McKay42/McOsu-NX](https://github.com/McKay42/McOsu-NX)

[https://github.com/McKay42/mph-model-viewer](https://github.com/McKay42/mph-model-viewer)

[Yesterday (university course project, YouTube link)](https://www.youtube.com/watch?v=RbuP1dNG304)